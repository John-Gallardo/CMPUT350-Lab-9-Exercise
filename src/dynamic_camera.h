#ifndef DYNAMIC_CAMERA_H
#define DYNAMIC_CAMERA_H

#include <iostream>
#include <limits>

#include "camera.h"
#include <iostream>

constexpr float INF = std::numeric_limits<float>::infinity();

// AABB in 1D
struct Interval {
    float min;
    float max;

    constexpr bool contains(const Interval& other) const {
        return (min <= other.min) && (max >= other.max);
    }

    constexpr bool contains(float x) const { return (min <= x) && (x <= max); }

    // signedDistance returns a positive smallest distance of x from the interval boundary
    // if it is not contained, else it returns a smallest (in magnitude) negative distance
    // from the interval boundary.
    constexpr float signedDistance(float x) const {
        if (contains(x)) {
            // 0 or negative
            return std::max(x - max, min - x);
        }
        // positive
        return std::max(x - max, min - x);
    }
};

struct DynamicCamera {
    void update(Camera& camera,                           // the camera to update
                const sf::Vector2f& trackedPosWorld,      // world coords of player to track
                const sf::Vector2f& trackedVelocityWorld  // world velocity of player to track
    ) {
        // TODO: (HINTS/outline)
        // collect statistics on the player position and velocity in (normalized, if you like)
        // window coordinates. Compute lookAhead distance in world coords, adjusting for
        // if the player is moving left or right.
        // Now update lookAhead state using an EMA over its previous state and the lookAhead world
        // vector. Then determine if the player is in the dead zone, soft rejection zone, or hard
        // rejection zone. For each, perform EMA on the delta of the camera displacement (ignoring
        // lookahead) appropriately. Next,
        // set the camera center by offsetting the cameraUnadjustedCenter, combining lookAhead.
        // Finally update cameraUnadjustedCenter, but without lookahead.
        
        // 1. collect statistics in WINDOW coordinates for player position and velocity
        sf::Vector2f trackedPosWindowPercent{camera.worldToWindowPercent(trackedPosWorld)};
        sf::Vector2f trackedVelocityWindowPercent{camera.worldToWindowPercentDist(trackedVelocityWorld)};

        // 2. compute lookahead distance in WORLD coordinates
        // NOTE: 0 for y since no lookahead for y
        sf::Vector2f lookAheadWorld{camera.windowPercentToWorldDist({lookAhead, 0.0f})};
        // convert to negative if going left
        if (trackedVelocityWorld.x < 0) {
            lookAheadWorld.x *= -1;
        }

        // 3. update lookAhead state using EMA
        // formula: positionNew = positionCurrent + alpha * (target - positionCurrent)
        // -> lookaheadState = lookaheadState + alpha * (lookAheadWorld - lookaheadState)
        // only use x since we lookahead for y is 0 anyway
        lookaheadState = lookaheadState + alpha * (lookAheadWorld.x - lookaheadState);

        // 4. determine if player is in dead zone, soft rejection, or hard rejection zone
        bool inDeadZone{deadZoneX.contains(trackedPosWindowPercent.x)};
        bool inSoftRejectionZone{softZoneX.contains(trackedPosWindowPercent.x) && !inDeadZone};  // in soft zone but not in deadzone
        // inHardRejectionZone is just the else block

        // 5. deal with the 3 cases
        static float cameraOffsetWorld{0.0f};  // static to keep track of state for EMA
        if (inDeadZone) {
            // camera doesn't move in deadzone
            cameraOffsetWorld = 0.0f;  // camera doesn't move
        } else if (inSoftRejectionZone) {
            /*
            // perform EMA
            if (trackedPosWindowPercent.x < deadZoneX.min) {
                // Case 1: on left soft rejection zone
                // formula: cameraOffsetWorld = cameraOffsetWorld + alpha * (cameraUnadjustedCenter + cameraOffsetWorld - cameraOffsetWorld)
                // -> cameraOffsetWorld = cameraOffsetWorld + alpha * cameraUnadjustedCenter
                cameraOffsetWorld = cameraOffsetWorld + alpha * cameraUnadjustedCenter.x;
            } else {
                // Case 2: on right soft rejection zone
                cameraOffsetWorld = cameraOffsetWorld + alpha * cameraUnadjustedCenter.x;
            }
            */
        } else {
            /*
            // camera offset in the hard rejection zone is just the difference between in X between the player position and the corresponding red line
            if (trackedPosWindowPercent.x < softZoneX.min) {
                // Case 1: in left hard zone
                cameraOffsetWorld = trackedPosWorld.x - camera.windowPercentToWorldDist({softZoneX.min, 0.0f}).x;  // wrap in brackets to transform to sf::Vector2f then just grab x.
            } else {
                // Case 2: in right hard zone
                cameraOffsetWorld = trackedPosWorld.x - camera.windowPercentToWorldDist({softZoneX.max, 0.0f}).x;
            }
            */
        }

        // 6. set camera center
        // NOTE: y offset is 0!!!
        camera.setCameraCenter(cameraUnadjustedCenter + sf::Vector2f{cameraOffsetWorld, 0.0f} + sf::Vector2f{lookaheadState, 0.0f});
        std::cout << lookaheadState << '\n';
        // std::cout << cameraOffsetWorld << '\n';

        // 7. update cameraUnadjustedCenter without lookahead
        cameraUnadjustedCenter = sf::Vector2f{cameraOffsetWorld, cameraUnadjustedCenter.y};
    }

    float alpha;  // alpha to use for damping of camera position, *as well as* for smoothing of
                  // lookahead offset.
    Interval deadZoneX;  // percentage interval along x-axis to use for dead zone in [0, 1]
    // percentage interval along x-axis to use for soft rejection zone in [0, 1]
    Interval softZoneX = {-INF, INF};
    // (everything beyond softZone is considered hard zone).
    float lookAhead;  // screen-percentage lookahead
    mutable sf::Vector2f
        cameraUnadjustedCenter;      // the camera world center from last frame, ignoring lookahead
    mutable float lookaheadState{};  // the EMA of the accumulated lookahead offset in world coords
};

#endif // DYNAMIC_CAMERA_H
