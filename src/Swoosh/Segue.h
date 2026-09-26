#pragma once
#include <SFML/Graphics.hpp>

#include "Timer.h"
#include "Activity.h"

namespace swoosh {
  class ActivityController;

  /**
   * @class quality
   * @brief Quality modes to toggle optimization in ActivityControllers and segue effects
   */
  enum class quality {
    realtime = 0, // Request real-time shader effects
    reduced,      // Request reduced, real-time shader effects
    mobile        // Request reduced and single-pass screen render. Also does not call onUpdate() on activities until complete.
  };

  /**
   * @class Segue
   * @brief A segue is a visual transition effect from one activity to another
   * 
   * Segues are activities that properly manage the life-cycle of the 2 activities: next and last
   * 
   * The only function that can be overriden is onDraw()
   * This allows programmers to create custom transition effects while only worrying about one thing: how it looks
   * This rigid design is intentional to influence plug-and-play segues. This way custom content can be shared and it "just works"
   */
  class Segue : public Activity {
    friend class ActivityController;

  private:
    Activity* last;
    Activity* next;
    sf::Time duration;
    Timer timer;

    // Hack to make this lib header-only
    void (ActivityController::*setActivityViewFunc)(sf::RenderTexture& surface, swoosh::Activity* activity);
    void (ActivityController::*resetViewFunc)(sf::RenderTexture& surface);

  protected:
    const sf::Time getDuration() const { return duration; }
    const sf::Time getElapsed() { return timer.getElapsed(); }
    const sf::Color getLastActivityBGColor() const { return last->getBGColor(); }
    const sf::Color getNextActivityBGColor() const { return next->getBGColor(); }

    void drawLastActivity(sf::RenderTexture& surface) {
      if (last) {
        (this->getController().*setActivityViewFunc)(surface, last);
        surface.clear(last->getBGColor());
        last->onDraw(surface);
        (this->getController().*resetViewFunc)(surface);
      }
    }

    void drawNextActivity(sf::RenderTexture& surface) {
      (this->getController().*setActivityViewFunc)(surface, next);
      surface.clear(next->getBGColor());
      next->onDraw(surface);
      (this->getController().*resetViewFunc)(surface);
    }

  public:
    void onStart() override final { next->onEnter();  last->onLeave(); timer.start(); }

void onUpdate (double elapsed) override final {
  timer.update(sf::seconds(static_cast<float>(elapsed)));

#if defined(__ANDROID__) || defined(ANDROID)
  // ONB_ANDROID_SWOOSH:
  // Android transition snapshots are cached/drawn during segues, so updating
  // the outgoing activity during the segue is unnecessary and can cause extra
  // work or visual state drift. Still update the incoming activity so it can
  // initialize animation/state before the transition completes.
  if (next) {
    next->onUpdate(elapsed);
  }
#else
  if (last) {
    last->onUpdate(elapsed);
  }

  if (next) {
    next->onUpdate(elapsed);
  }
#endif
}

    void onLeave() override final { timer.pause(); }
    void onExit() override final { ; }
    void onEnter() override final { ; }
    void onResume() override final { timer.reset(); timer.start(); }
    virtual void onDraw(sf::RenderTexture& surface) = 0;
    void onEnd() override final { last->onExit(); }

    Segue() = delete;
    Segue(sf::Time duration, Activity* last, Activity* next) 
      : setActivityViewFunc(nullptr), resetViewFunc(nullptr), duration(duration), last(last), next(next), Activity(&next->getController()) { /* ... */ }
    virtual ~Segue() { }
  };
}
