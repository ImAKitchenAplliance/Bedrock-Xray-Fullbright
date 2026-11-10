#pragma once
#include <array>
#include <cstdint>
namespace titanium {
enum class Session { Unknown, Menu, LocalWorld, Realm, DedicatedServer };
enum class Feature : unsigned { Xray, NoFall, Fly, Count };
struct Evidence {
    Session session = Session::Unknown;
    bool verified_build = false;
    std::uint64_t generation = 0;
    std::array<bool, 3> supported{};
};
class Backend {
public:
    virtual ~Backend() = default;
    virtual bool enable(Feature) = 0;
    virtual void disable(Feature) noexcept = 0;
};
class Runtime {
    Backend& backend_;
    Evidence evidence_{};
    std::array<bool, 3> active_{};
    bool allowed(unsigned i) const {
        return evidence_.verified_build && evidence_.generation && evidence_.supported[i] &&
            (evidence_.session == Session::LocalWorld || evidence_.session == Session::Realm);
    }
public:
    explicit Runtime(Backend& b) : backend_(b) {}
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    ~Runtime() { clear(); }
    void clear() noexcept {
        for (unsigned i=0;i<3;++i) {
            if (active_[i]) backend_.disable(static_cast<Feature>(i));
            active_[i] = false;
        }
    }
    bool set(Feature f, bool enabled) {
        const auto i=static_cast<unsigned>(f);
        if (i>=3) return false;
        if (!enabled) {
            if (active_[i]) backend_.disable(f);
            active_[i]=false;
            return true;
        }
        if (!allowed(i)) return false;
        if (!active_[i]) active_[i]=backend_.enable(f);
        return active_[i];
    }
    void update(Evidence e) {
        if (e.session!=evidence_.session || e.generation!=evidence_.generation) clear();
        evidence_=e;
        for (unsigned i=0;i<3;++i) if (active_[i] && !allowed(i)) set(static_cast<Feature>(i),false);
    }
};
}
