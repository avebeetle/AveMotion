#include "avemotion/player/Player.hpp"
#include "PlayerSource.hpp"

#include "avemotion/runtime/Runtime.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace avemotion::player::detail {
namespace {

[[nodiscard]] constexpr bool timeLess(
    runtime::MotionTime left,
    runtime::MotionTime right) noexcept {
    return left.nanoseconds < right.nanoseconds;
}

[[nodiscard]] constexpr bool timeLessEqual(
    runtime::MotionTime left,
    runtime::MotionTime right) noexcept {
    return left.nanoseconds <= right.nanoseconds;
}

[[nodiscard]] runtime::MotionTime saturatingAdd(
    runtime::MotionTime time,
    std::int64_t delta) noexcept {
    if (delta > 0
        && time.nanoseconds > std::numeric_limits<std::int64_t>::max() - delta) {
        return runtime::MotionTime::fromNanoseconds(
            std::numeric_limits<std::int64_t>::max());
    }
    if (delta < 0
        && time.nanoseconds < std::numeric_limits<std::int64_t>::min() - delta) {
        return runtime::MotionTime::fromNanoseconds(
            std::numeric_limits<std::int64_t>::min());
    }
    return runtime::MotionTime::fromNanoseconds(time.nanoseconds + delta);
}

[[nodiscard]] std::optional<std::int64_t> frameIntervalNanoseconds(
    const PlayerSource& source,
    const runtime::PlaybackSnapshot& snapshot,
    double maximumPresentationRate) noexcept {
    const auto sourceRate = source.ops->frameRate(source.owner.get());
    if (!std::isfinite(sourceRate) || sourceRate <= 0.0
        || !std::isfinite(snapshot.playbackRate) || snapshot.playbackRate <= 0.0
        || !std::isfinite(maximumPresentationRate)
        || maximumPresentationRate <= 0.0) {
        return std::nullopt;
    }
    const auto effective = std::min(
        sourceRate * snapshot.playbackRate,
        maximumPresentationRate);
    if (!std::isfinite(effective) || effective <= 0.0) {
        return std::nullopt;
    }
    constexpr long double nanosecondsPerSecond = 1'000'000'000.0L;
    const auto interval = nanosecondsPerSecond
        / static_cast<long double>(effective);
    if (interval >= static_cast<long double>(
            std::numeric_limits<std::int64_t>::max())) {
        return std::numeric_limits<std::int64_t>::max();
    }
    return std::max<std::int64_t>(
        1,
        static_cast<std::int64_t>(std::llround(interval)));
}

} // namespace

struct PlayerEntry final {
    std::uint32_t generation = 0U;
    bool occupied = false;
    PlayerSource source;
    PlayerEntryOptions options;
    FrameReason pendingReasons = FrameReason::None;
    bool automaticPause = false;
    bool hasDeadline = false;
    runtime::MotionTime deadline;
    std::uint64_t lastPlaybackRevision = 0U;
    bool lastCompleted = false;
};

struct PlayerState final {
    explicit PlayerState(PlayerHostCallbacks value) noexcept
        : callbacks(value) {
    }

    ~PlayerState() {
        // A host adapter may have one outstanding platform wakeup for the
        // published deadline. Retiring or move-assigning a Player must release
        // that wakeup so it cannot later target a dead scheduling object.
        if (publishedDeadline.has_value()
            && callbacks.cancelWakeup != nullptr
            && !dispatchingHostCallback) {
            callbacks.cancelWakeup(callbacks.userData);
        }
    }

    PlayerHostCallbacks callbacks;
    std::vector<PlayerEntry> entries;
    std::vector<std::uint32_t> freeIndices;
    std::vector<ScheduledFrame> scheduledFrames;
    std::optional<runtime::MotionTime> publishedDeadline;
    bool frameRequestPending = false;
    bool dispatchingHostCallback = false;
    std::uint64_t tickSequence = 0U;
    PlayerDiagnostics counters;

    [[nodiscard]] PlayerEntry* find(PlayerHandle handle) noexcept {
        if (!handle.valid() || handle.index >= entries.size()) return nullptr;
        auto& entry = entries[handle.index];
        return entry.occupied && entry.generation == handle.generation
            ? &entry
            : nullptr;
    }

    [[nodiscard]] const PlayerEntry* find(PlayerHandle handle) const noexcept {
        if (!handle.valid() || handle.index >= entries.size()) return nullptr;
        const auto& entry = entries[handle.index];
        return entry.occupied && entry.generation == handle.generation
            ? &entry
            : nullptr;
    }

    void accountStorageChange(
        std::size_t oldEntriesCapacity,
        std::size_t oldFreeCapacity,
        std::size_t oldFramesCapacity) noexcept {
        if (entries.capacity() != oldEntriesCapacity
            || freeIndices.capacity() != oldFreeCapacity
            || scheduledFrames.capacity() != oldFramesCapacity) {
            ++counters.storageGeneration;
        }
    }

    void requestHostFrame() noexcept {
        if (frameRequestPending) {
            ++counters.frameRequestsCoalesced;
            return;
        }
        frameRequestPending = true;
        ++counters.frameRequestsPosted;
        if (callbacks.requestFrame == nullptr || dispatchingHostCallback) return;
        dispatchingHostCallback = true;
        callbacks.requestFrame(callbacks.userData);
        dispatchingHostCallback = false;
    }

    void publishDeadline(std::optional<runtime::MotionTime> deadline) noexcept {
        if (publishedDeadline == deadline) return;
        publishedDeadline = deadline;
        if (dispatchingHostCallback) return;
        dispatchingHostCallback = true;
        if (deadline.has_value()) {
            ++counters.wakeupsScheduled;
            if (callbacks.scheduleWakeup != nullptr) {
                callbacks.scheduleWakeup(callbacks.userData, *deadline);
            }
        } else {
            ++counters.wakeupsCancelled;
            if (callbacks.cancelWakeup != nullptr) {
                callbacks.cancelWakeup(callbacks.userData);
            }
        }
        dispatchingHostCallback = false;
    }

    [[nodiscard]] std::optional<runtime::MotionTime> computeEarliestDeadline()
        const noexcept {
        std::optional<runtime::MotionTime> earliest;
        for (const auto& entry : entries) {
            if (!entry.occupied || !entry.options.visible || !entry.hasDeadline) {
                continue;
            }
            if (!earliest.has_value() || timeLess(entry.deadline, *earliest)) {
                earliest = entry.deadline;
            }
        }
        return earliest;
    }

    void updateCurrentCounts(runtime::MotionTime now) noexcept {
        std::size_t registered = 0U;
        std::size_t visible = 0U;
        std::size_t playing = 0U;
        for (const auto& entry : entries) {
            if (!entry.occupied || !entry.source.owner) continue;
            ++registered;
            if (entry.options.visible) ++visible;
            if (entry.source.ops->snapshot(entry.source.owner.get(), now).status
                == runtime::PlaybackStatus::Playing) {
                ++playing;
            }
        }
        counters.registeredInstances = registered;
        counters.visibleInstances = visible;
        counters.playingInstances = playing;
    }

    void resetCadence(PlayerEntry& entry, runtime::MotionTime now) noexcept {
        entry.hasDeadline = false;
        if (!entry.source.owner || !entry.options.visible) return;
        const auto snapshot = entry.source.ops->snapshot(entry.source.owner.get(), now);
        if (snapshot.status != runtime::PlaybackStatus::Playing) return;
        const auto interval = frameIntervalNanoseconds(
            entry.source,
            snapshot,
            entry.options.maximumPresentationRate);
        if (!interval.has_value()) return;
        entry.deadline = saturatingAdd(now, *interval);
        entry.hasDeadline = true;
    }

    void afterMutation(runtime::MotionTime now, bool requestFrame) noexcept {
        if (requestFrame) requestHostFrame();
        publishDeadline(computeEarliestDeadline());
        updateCurrentCounts(now);
    }
};

} // namespace avemotion::player::detail

namespace avemotion::player {
namespace {

[[nodiscard]] detail::PlayerEntry* validEntry(
    detail::PlayerState& state,
    PlayerHandle handle) noexcept {
    return state.find(handle);
}

void enforceHiddenFreeze(
    detail::PlayerState& state,
    detail::PlayerEntry& entry,
    runtime::MotionTime now) noexcept {
    if (entry.options.visible
        || entry.options.hiddenTimePolicy != HiddenTimePolicy::Freeze
        || entry.automaticPause) {
        return;
    }
    if (entry.source.ops->snapshot(entry.source.owner.get(), now).status
        == runtime::PlaybackStatus::Playing) {
        entry.source.ops->pause(entry.source.owner.get(), now);
        entry.automaticPause = true;
        ++state.counters.automaticPauses;
    }
}

void markPlaybackMutation(
    detail::PlayerState& state,
    detail::PlayerEntry& entry,
    runtime::MotionTime now) noexcept {
    entry.pendingReasons |= FrameReason::PlaybackChanged;
    state.resetCadence(entry, now);
    state.afterMutation(now, entry.options.visible);
}

} // namespace

Player::Player(PlayerHostCallbacks callbacks)
    : state_(std::make_unique<detail::PlayerState>(callbacks)) {
}

Player::Player(Player&&) noexcept = default;
Player& Player::operator=(Player&&) noexcept = default;
Player::~Player() = default;

void Player::setHostCallbacks(
    PlayerHostCallbacks callbacks,
    runtime::MotionTime now) noexcept {
    const auto hadPendingFrameRequest = state_->frameRequestPending;
    if (state_->publishedDeadline.has_value()
        && state_->callbacks.cancelWakeup != nullptr
        && !state_->dispatchingHostCallback) {
        state_->dispatchingHostCallback = true;
        ++state_->counters.wakeupsCancelled;
        state_->callbacks.cancelWakeup(state_->callbacks.userData);
        state_->dispatchingHostCallback = false;
    }
    state_->callbacks = callbacks;
    state_->frameRequestPending = false;
    state_->publishedDeadline.reset();
    const auto hasPendingEntry = std::any_of(
        state_->entries.begin(),
        state_->entries.end(),
        [](const auto& entry) {
            return entry.occupied
                && entry.pendingReasons != FrameReason::None;
        });
    // A pending host repaint can outlive the entry that caused it (for
    // example, removeInstance() still needs the host to clear old pixels).
    // Replacing callbacks must therefore preserve the coalesced request even
    // when no live entry has a pending reason anymore.
    if (hadPendingFrameRequest || hasPendingEntry) state_->requestHostFrame();
    state_->publishDeadline(state_->computeEarliestDeadline());
    state_->updateCurrentCounts(now);
}

PlayerAddResult Player::addInstance(
    std::shared_ptr<runtime::Instance> instance,
    runtime::MotionTime now,
    PlayerEntryOptions options) {
    if (!instance) {
        return {{}, {PlayerErrorCode::InvalidArgument,
                     "Player instance must not be null"}};
    }
    static const detail::PlayerSourceOps runtimeOps{
        .frameRate = [](const void* object) noexcept {
            return static_cast<const runtime::Instance*>(object)
                ->assetMetadata().frameRate;
        },
        .snapshot = [](const void* object, runtime::MotionTime time) noexcept {
            return static_cast<const runtime::Instance*>(object)
                ->playbackSnapshot(time);
        },
        .play = [](void* object, runtime::MotionTime time) noexcept {
            static_cast<runtime::Instance*>(object)->play(time);
        },
        .pause = [](void* object, runtime::MotionTime time) noexcept {
            static_cast<runtime::Instance*>(object)->pause(time);
        },
        .resume = [](void* object, runtime::MotionTime time) noexcept {
            static_cast<runtime::Instance*>(object)->resume(time);
        },
        .stop = [](void* object) noexcept {
            static_cast<runtime::Instance*>(object)->stop();
        },
        .seekNormalized = [](void* object, double value,
                             runtime::MotionTime time) noexcept {
            static_cast<runtime::Instance*>(object)->seekNormalized(value, time);
        },
        .setControlledProgress = [](void* object, double value) noexcept {
            static_cast<runtime::Instance*>(object)->setControlledProgress(value);
        },
        .setDirection = [](void* object, runtime::PlaybackDirection value,
                           runtime::MotionTime time) noexcept {
            static_cast<runtime::Instance*>(object)->setDirection(value, time);
        },
        .setPlaybackRate = [](void* object, double value,
                              runtime::MotionTime time) noexcept {
            return static_cast<runtime::Instance*>(object)
                ->setPlaybackRate(value, time);
        },
        .setLoopMode = [](void* object,
                          runtime::PlaybackLoopMode value) noexcept {
            static_cast<runtime::Instance*>(object)->setLoopMode(value);
        },
    };
    detail::PlayerSource source{
        .owner = instance,
        .ops = &runtimeOps,
        .runtimeInstance = instance.get(),
        .runtimeHandle = instance->handle(),
    };
    return detail::PlayerSourceAccess::add(*this, std::move(source), now, options);
}

bool Player::removeInstance(
    PlayerHandle handle,
    runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->occupied = false;
    entry->source = {};
    entry->pendingReasons = FrameReason::None;
    entry->automaticPause = false;
    entry->hasDeadline = false;
    state_->freeIndices.push_back(handle.index);
    ++state_->counters.removals;
    state_->afterMutation(now, true);
    return true;
}

void Player::clear(runtime::MotionTime now) noexcept {
    bool removed = false;
    for (std::uint32_t index = 0U; index < state_->entries.size(); ++index) {
        auto& entry = state_->entries[index];
        if (!entry.occupied) continue;
        entry.occupied = false;
        entry.source = {};
        entry.hasDeadline = false;
        entry.pendingReasons = FrameReason::None;
        state_->freeIndices.push_back(index);
        ++state_->counters.removals;
        removed = true;
    }
    state_->afterMutation(now, removed);
}

std::shared_ptr<runtime::Instance> Player::instance(
    PlayerHandle handle) const noexcept {
    const auto* entry = state_->find(handle);
    if (entry == nullptr || entry->source.runtimeInstance == nullptr) return {};
    return std::shared_ptr<runtime::Instance>{
        entry->source.owner, entry->source.runtimeInstance};
}

bool Player::setVisible(
    PlayerHandle handle,
    bool visible,
    runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr || entry->options.visible == visible) return entry != nullptr;

    if (!visible
        && entry->options.hiddenTimePolicy == HiddenTimePolicy::Freeze) {
        const auto snapshot = entry->source.ops->snapshot(entry->source.owner.get(), now);
        if (snapshot.status == runtime::PlaybackStatus::Playing) {
            entry->source.ops->pause(entry->source.owner.get(), now);
            entry->automaticPause = true;
            ++state_->counters.automaticPauses;
        }
    }
    entry->options.visible = visible;
    entry->pendingReasons |= FrameReason::VisibilityChanged;
    if (visible && entry->automaticPause) {
        entry->source.ops->resume(entry->source.owner.get(), now);
        entry->automaticPause = false;
        ++state_->counters.automaticResumes;
    }
    state_->resetCadence(*entry, now);
    ++state_->counters.visibilityChanges;
    // Hiding also requests one host frame so the old presentation bounds can
    // be cleared by the host adapter.
    state_->afterMutation(now, true);
    return true;
}

bool Player::setMaximumPresentationRate(
    PlayerHandle handle,
    double framesPerSecond,
    runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr || !std::isfinite(framesPerSecond)
        || framesPerSecond <= 0.0) {
        return false;
    }
    entry->options.maximumPresentationRate = framesPerSecond;
    entry->pendingReasons |= FrameReason::PlaybackChanged;
    state_->resetCadence(*entry, now);
    state_->afterMutation(now, entry->options.visible);
    return true;
}

bool Player::invalidate(PlayerHandle handle, FrameReason reason) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    if (reason == FrameReason::None) reason = FrameReason::ExplicitInvalidation;
    entry->pendingReasons |= reason;
    state_->requestHostFrame();
    return true;
}

bool Player::play(PlayerHandle handle, runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->automaticPause = false;
    entry->source.ops->play(entry->source.owner.get(), now);
    enforceHiddenFreeze(*state_, *entry, now);
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

bool Player::pause(PlayerHandle handle, runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->automaticPause = false;
    entry->source.ops->pause(entry->source.owner.get(), now);
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

bool Player::resume(PlayerHandle handle, runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->automaticPause = false;
    entry->source.ops->resume(entry->source.owner.get(), now);
    enforceHiddenFreeze(*state_, *entry, now);
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

bool Player::stop(PlayerHandle handle, runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->automaticPause = false;
    entry->source.ops->stop(entry->source.owner.get());
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

bool Player::seekNormalized(
    PlayerHandle handle,
    double normalizedPosition,
    runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->source.ops->seekNormalized(entry->source.owner.get(), normalizedPosition, now);
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

bool Player::setControlledProgress(
    PlayerHandle handle,
    double normalizedPosition,
    runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->automaticPause = false;
    entry->source.ops->setControlledProgress(entry->source.owner.get(), normalizedPosition);
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

bool Player::setDirection(
    PlayerHandle handle,
    runtime::PlaybackDirection direction,
    runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->source.ops->setDirection(entry->source.owner.get(), direction, now);
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

bool Player::setPlaybackRate(
    PlayerHandle handle,
    double playbackRate,
    runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr
        || !entry->source.ops->setPlaybackRate(
            entry->source.owner.get(), playbackRate, now)) {
        return false;
    }
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

bool Player::setLoopMode(
    PlayerHandle handle,
    runtime::PlaybackLoopMode loopMode,
    runtime::MotionTime now) noexcept {
    auto* entry = validEntry(*state_, handle);
    if (entry == nullptr) return false;
    entry->source.ops->setLoopMode(entry->source.owner.get(), loopMode);
    markPlaybackMutation(*state_, *entry, now);
    return true;
}

PlayerTickView Player::tick(
    runtime::MotionTime now,
    FrameSelection selection) noexcept {
    state_->frameRequestPending = false;
    state_->scheduledFrames.clear();
    ++state_->counters.ticks;
    ++state_->tickSequence;
    std::uint64_t skippedThisTick = 0U;

    for (std::uint32_t index = 0U; index < state_->entries.size(); ++index) {
        auto& entry = state_->entries[index];
        if (!entry.occupied || !entry.source.owner) continue;

        auto snapshot = entry.source.ops->snapshot(entry.source.owner.get(), now);
        auto reasons = entry.pendingReasons;
        const auto revisionChanged =
            snapshot.revision != entry.lastPlaybackRevision;
        if (revisionChanged) {
            reasons |= FrameReason::PlaybackChanged;
            state_->resetCadence(entry, now);
        }

        if (entry.options.visible) {
            if (snapshot.status == runtime::PlaybackStatus::Playing) {
                const auto interval = detail::frameIntervalNanoseconds(
                    entry.source,
                    snapshot,
                    entry.options.maximumPresentationRate);
                if (interval.has_value()) {
                    if (!entry.hasDeadline) {
                        entry.deadline = detail::saturatingAdd(now, *interval);
                        entry.hasDeadline = true;
                    } else if (detail::timeLessEqual(entry.deadline, now)) {
                        reasons |= FrameReason::TimelineAdvanced;
                        const auto lateness =
                            static_cast<long double>(now.nanoseconds)
                            - static_cast<long double>(entry.deadline.nanoseconds);
                        const auto periodEstimate = std::floor(
                            lateness / static_cast<long double>(*interval)) + 1.0L;
                        const auto periods = periodEstimate
                                >= static_cast<long double>(
                                    std::numeric_limits<std::uint64_t>::max())
                            ? std::numeric_limits<std::uint64_t>::max()
                            : static_cast<std::uint64_t>(periodEstimate);
                        if (periods > 1U) {
                            const auto skipped = periods - 1U;
                            skippedThisTick += skipped;
                            state_->counters.skippedDeadlines += skipped;
                        }
                        const auto maxPeriods = static_cast<std::uint64_t>(
                            std::numeric_limits<std::int64_t>::max()
                            / *interval);
                        const auto safePeriods = std::min(periods, maxPeriods);
                        entry.deadline = detail::saturatingAdd(
                            entry.deadline,
                            static_cast<std::int64_t>(safePeriods) * *interval);
                        if (safePeriods != periods) {
                            entry.deadline = runtime::MotionTime::fromNanoseconds(
                                std::numeric_limits<std::int64_t>::max());
                        }
                    }
                } else {
                    entry.hasDeadline = false;
                }
            } else {
                entry.hasDeadline = false;
            }

            if (selection == FrameSelection::AllVisible) {
                reasons |= FrameReason::HostRepaint;
            }
        } else {
            entry.hasDeadline = false;
        }

        if (snapshot.completed && !entry.lastCompleted) {
            reasons |= FrameReason::Completion;
        }

        if (reasons != FrameReason::None
            && (entry.options.visible
                || hasReason(reasons, FrameReason::VisibilityChanged))) {
            state_->scheduledFrames.push_back({
                .handle = {index, entry.generation},
                .instanceHandle = entry.source.runtimeInstance != nullptr
                    ? entry.source.runtimeInstance->handle()
                    : runtime::InstanceHandle{},
                .instance = entry.source.runtimeInstance,
                .playback = snapshot,
                .reasons = reasons,
                .visible = entry.options.visible,
            });
        }

        entry.pendingReasons = FrameReason::None;
        entry.lastPlaybackRevision = snapshot.revision;
        entry.lastCompleted = snapshot.completed;
    }

    state_->counters.framesReturned += state_->scheduledFrames.size();
    state_->publishDeadline(state_->computeEarliestDeadline());
    state_->updateCurrentCounts(now);
    return {
        .frames = state_->scheduledFrames,
        .nextDeadline = state_->publishedDeadline,
        .sequence = state_->tickSequence,
        .skippedDeadlines = skippedThisTick,
    };
}

std::optional<runtime::MotionTime> Player::nextDeadline() const noexcept {
    return state_->publishedDeadline;
}

PlayerDiagnostics Player::diagnostics() const noexcept {
    return state_->counters;
}

void Player::resetDiagnostics() noexcept {
    const auto registered = state_->counters.registeredInstances;
    const auto visible = state_->counters.visibleInstances;
    const auto playing = state_->counters.playingInstances;
    const auto storage = state_->counters.storageGeneration;
    state_->counters = {};
    state_->counters.registeredInstances = registered;
    state_->counters.visibleInstances = visible;
    state_->counters.playingInstances = playing;
    state_->counters.storageGeneration = storage;
}

} // namespace avemotion::player

namespace avemotion::player::detail {

PlayerAddResult PlayerSourceAccess::add(
    Player& player, PlayerSource source, runtime::MotionTime now,
    PlayerEntryOptions options) {
    PlayerAddResult result;
    if (!source.owner || source.ops == nullptr
        || source.ops->frameRate == nullptr
        || source.ops->snapshot == nullptr
        || source.ops->play == nullptr
        || source.ops->pause == nullptr
        || source.ops->resume == nullptr
        || source.ops->stop == nullptr
        || source.ops->seekNormalized == nullptr
        || source.ops->setControlledProgress == nullptr
        || source.ops->setDirection == nullptr
        || source.ops->setPlaybackRate == nullptr
        || source.ops->setLoopMode == nullptr) {
        result.error = {
            PlayerErrorCode::InvalidArgument,
            "Player source must have an owner and complete operations"};
        return result;
    }
    if (!std::isfinite(options.maximumPresentationRate)
        || options.maximumPresentationRate <= 0.0) {
        result.error = {
            PlayerErrorCode::InvalidArgument,
            "maximumPresentationRate must be finite and positive"};
        return result;
    }
    auto& state = *player.state_;
    for (const auto& entry : state.entries) {
        if (entry.occupied && entry.source.owner.get() == source.owner.get()) {
            result.error = {
                PlayerErrorCode::DuplicateInstance,
                source.runtimeInstance != nullptr
                    ? "The runtime instance is already registered with this player"
                    : "The player source is already registered with this player"};
            return result;
        }
    }

    const auto oldEntriesCapacity = state.entries.capacity();
    const auto oldFreeCapacity = state.freeIndices.capacity();
    const auto oldFramesCapacity = state.scheduledFrames.capacity();

    std::uint32_t index = 0U;
    if (!state.freeIndices.empty()) {
        index = state.freeIndices.back();
        state.freeIndices.pop_back();
    } else {
        if (state.entries.size()
            >= static_cast<std::size_t>(kInvalidPlayerHandleIndex)) {
            result.error = {
                PlayerErrorCode::InvalidArgument,
                "Player handle space is exhausted"};
            return result;
        }
        index = static_cast<std::uint32_t>(state.entries.size());
        state.entries.emplace_back();
    }

    auto& entry = state.entries[index];
    entry.generation = entry.generation == std::numeric_limits<std::uint32_t>::max()
        ? 1U
        : entry.generation + 1U;
    if (entry.generation == 0U) entry.generation = 1U;
    entry.occupied = true;
    entry.source = std::move(source);
    entry.options = options;
    entry.pendingReasons = FrameReason::FirstFrame;
    entry.automaticPause = false;
    entry.hasDeadline = false;
    entry.lastPlaybackRevision = 0U;
    entry.lastCompleted = false;

    if (state.scheduledFrames.capacity() < state.entries.size()) {
        state.scheduledFrames.reserve(state.entries.size());
    }
    if (state.freeIndices.capacity() < state.entries.size()) {
        state.freeIndices.reserve(state.entries.size());
    }
    state.accountStorageChange(
        oldEntriesCapacity,
        oldFreeCapacity,
        oldFramesCapacity);

    enforceHiddenFreeze(state, entry, now);
    state.resetCadence(entry, now);
    ++state.counters.registrations;
    result.handle = {index, entry.generation};
    state.afterMutation(now, true);
    return result;
}

PlayerSource PlayerSourceAccess::source(
    const Player& player, PlayerHandle handle) noexcept {
    const auto* entry = player.state_->find(handle);
    return entry != nullptr ? entry->source : PlayerSource{};
}

} // namespace avemotion::player::detail
