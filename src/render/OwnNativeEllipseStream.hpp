#pragma once

#include "OwnNativeEllipsePreparedAsset.hpp"
#include "avemotion/evaluation/PropertyEvaluator.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace avemotion::render::detail {

enum class OwnNativeEllipseCreateCode {
    Ready, InvalidPreparedAsset, InvalidSourceIdentity, IdentityExhausted,
    EvaluationPreparationFailed
};

enum class OwnNativeEllipseFrameCode {
    Emitted, SequenceExhausted, InvalidViewport, EvaluationFailed,
    UnsupportedNumericOutput, ModelApplicationFailed
};

class OwnNativeEllipseStream;

struct OwnNativeEllipseCreateResult final {
    OwnNativeEllipseCreateCode code = OwnNativeEllipseCreateCode::InvalidPreparedAsset;
    std::string message;
    std::unique_ptr<OwnNativeEllipseStream> stream;
    [[nodiscard]] explicit operator bool() const noexcept {
        return code == OwnNativeEllipseCreateCode::Ready && stream != nullptr;
    }
};

struct OwnNativeEllipseFrameResult final {
    OwnNativeEllipseFrameCode code = OwnNativeEllipseFrameCode::EvaluationFailed;
    std::string message;
    std::optional<runtime::EvaluatedScene> scene;
    [[nodiscard]] explicit operator bool() const noexcept {
        return code == OwnNativeEllipseFrameCode::Emitted && scene.has_value();
    }
};

class OwnNativeEllipseStream final {
public:
    [[nodiscard]] static OwnNativeEllipseCreateResult create(
        std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared);
    [[nodiscard]] OwnNativeEllipseFrameResult emit(
        std::size_t frame, std::size_t width, std::size_t height);
    OwnNativeEllipseStream(const OwnNativeEllipseStream&) = delete;
    OwnNativeEllipseStream& operator=(const OwnNativeEllipseStream&) = delete;
    OwnNativeEllipseStream(OwnNativeEllipseStream&&) = delete;
    OwnNativeEllipseStream& operator=(OwnNativeEllipseStream&&) = delete;
    ~OwnNativeEllipseStream();

private:
    explicit OwnNativeEllipseStream(
        std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared,
        std::uint64_t identity);
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared_;
    std::uint64_t identity_ = 0;
    evaluation::PropertyEvaluator evaluator_;
    evaluation::PropertyEvaluationWorkspace workspace_;
    std::uint64_t attemptSequence_ = 0;
    runtime::SceneFingerprints previous_;
    bool hasPrevious_ = false;
};

} // namespace avemotion::render::detail
