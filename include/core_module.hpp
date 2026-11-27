#pragma once

namespace rotm
{
class Core;

class CoreModule
{
public:
    CoreModule(Core& coreRef) : core(coreRef)
    {
    }

    virtual ~CoreModule() = default;

    CoreModule() = delete;
    CoreModule(const CoreModule&) = delete;
    CoreModule(CoreModule&&) = delete;
    CoreModule& operator=(const CoreModule&) = delete;
    CoreModule& operator=(CoreModule&&) = delete;

    virtual void update()
    {
    }

    virtual void tick()
    {
    }

    virtual void draw()
    {
    }

    virtual void sync_gui()
    {
    }

    virtual bool want_exit() const
    {
        return m_wantExit;
    }

protected:
    Core& core;
    bool m_wantExit = false;
};
} // namespace rotm
