#include "Player.h"

#include "../Physics/Collision.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

void Player::Reset(const glm::vec3& Position)
{
    PlayerPosition = Position;
    PlayerPosition.y = 1.65f;

    Yaw = 0.0f;
    Pitch = 0.0f;
    TargetYaw = 0.0f;
    TargetPitch = 0.0f;

    MouseDeltaX = 0.0f;
    MouseDeltaY = 0.0f;
    MouseCapturedLastFrame = false;
    IgnoreMouseMotionEvents = 0;

    Bob = 0.0f;
    BobTime = 0.0f;
    Roll = 0.0f;

    CurrentStamina = 1.0f;
    CurrentFov = 73.0f;
}

void Player::HandleEvent(const SDL_Event& Event, bool MouseCaptured)
{
    if (!MouseCaptured)
        return;

    if (Event.type == SDL_EVENT_MOUSE_MOTION)
    {
        if (IgnoreMouseMotionEvents > 0)
        {
            --IgnoreMouseMotionEvents;
            return;
        }

        const float DeltaX = std::clamp(
            static_cast<float>(Event.motion.xrel),
            -120.0f,
            120.0f
        );

        const float DeltaY = std::clamp(
            static_cast<float>(Event.motion.yrel),
            -120.0f,
            120.0f
        );

        MouseDeltaX = std::clamp(
            MouseDeltaX + DeltaX,
            -240.0f,
            240.0f
        );

        MouseDeltaY = std::clamp(
            MouseDeltaY + DeltaY,
            -240.0f,
            240.0f
        );
    }
}

void Player::OnMouseCaptureChanged(bool Captured)
{
    MouseCapturedLastFrame = Captured;
    MouseDeltaX = 0.0f;
    MouseDeltaY = 0.0f;

    TargetYaw = Yaw;
    TargetPitch = Pitch;

    if (Captured)
        IgnoreMouseMotionEvents = 2;
    else
        IgnoreMouseMotionEvents = 0;
}

glm::vec3 Player::Forward() const
{
    const float CosPitch = std::cos(Pitch);

    return glm::normalize(glm::vec3{
        -std::sin(Yaw) * CosPitch,
        std::sin(Pitch),
        -std::cos(Yaw) * CosPitch
    });
}

glm::vec3 Player::Right() const
{
    return glm::normalize(glm::vec3{
        std::cos(Yaw),
        0.0f,
        -std::sin(Yaw)
    });
}

void Player::Update(
    float DeltaTime,
    const std::vector<AABB>& Colliders,
    bool MouseCaptured
)
{
    if (MouseCaptured != MouseCapturedLastFrame)
        OnMouseCaptureChanged(MouseCaptured);

    if (MouseCaptured)
    {
        TargetYaw -= MouseDeltaX * 0.00175f;
        TargetPitch -= MouseDeltaY * 0.00165f;
        TargetPitch = std::clamp(TargetPitch, -1.45f, 1.45f);
    }

    MouseDeltaX = 0.0f;
    MouseDeltaY = 0.0f;

    const float LookBlend =
        1.0f - std::exp(-DeltaTime * 28.0f);

    Yaw += (TargetYaw - Yaw) * LookBlend;
    Pitch += (TargetPitch - Pitch) * LookBlend;

    const bool* Keys = SDL_GetKeyboardState(nullptr);

    const float ForwardInput =
        (Keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) -
        (Keys[SDL_SCANCODE_S] ? 1.0f : 0.0f);

    const float SideInput =
        (Keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) -
        (Keys[SDL_SCANCODE_A] ? 1.0f : 0.0f);

    const bool Moving =
        std::abs(ForwardInput) > 0.001f ||
        std::abs(SideInput) > 0.001f;

    const bool WantsSprint =
        (Keys[SDL_SCANCODE_LSHIFT] || Keys[SDL_SCANCODE_RSHIFT]) &&
        ForwardInput > 0.0f &&
        CurrentStamina > 0.03f;

    const float Speed =
        WantsSprint
            ? SprintSpeed
            : WalkSpeed;

    if (WantsSprint && Moving)
        CurrentStamina = std::max(
            0.0f,
            CurrentStamina - DeltaTime * 0.18f
        );
    else
        CurrentStamina = std::min(
            1.0f,
            CurrentStamina + DeltaTime * 0.13f
        );

    glm::vec3 HorizontalForward{
        -std::sin(Yaw),
        0.0f,
        -std::cos(Yaw)
    };

    glm::vec3 HorizontalRight{
        std::cos(Yaw),
        0.0f,
        -std::sin(Yaw)
    };

    glm::vec3 Move =
        HorizontalForward * ForwardInput +
        HorizontalRight * SideInput;

    if (glm::dot(Move, Move) > 0.000001f)
    {
        Move = glm::normalize(Move) * Speed * DeltaTime;

        PlayerPosition = Collision::ResolveHorizontalMove(
            PlayerPosition,
            Move,
            Radius,
            Colliders
        );
    }

    const float MotionBlend =
        1.0f - std::exp(-DeltaTime * 14.0f);

    if (Moving)
    {
        BobTime += DeltaTime * (
            WantsSprint
                ? 10.8f
                : 7.8f
        );
    }

    const float TargetBob =
        Moving
            ? std::sin(BobTime * 2.0f) *
                (WantsSprint ? 0.026f : 0.018f)
            : 0.0f;

    const float TargetRoll =
        Moving
            ? -SideInput * 0.012f +
                std::sin(BobTime) * 0.004f
            : 0.0f;

    Bob += (TargetBob - Bob) * MotionBlend;
    Roll += (TargetRoll - Roll) * MotionBlend;

    const float TargetFov =
        WantsSprint && Moving
            ? 76.0f
            : 73.0f;

    CurrentFov +=
        (TargetFov - CurrentFov) *
        (1.0f - std::exp(-DeltaTime * 8.0f));
}

glm::mat4 Player::ViewMatrix() const
{
    glm::vec3 Eye = PlayerPosition;
    Eye.y += Bob;

    glm::mat4 View{1.0f};

    View = glm::rotate(
        View,
        -Roll,
        glm::vec3{0.0f, 0.0f, 1.0f}
    );

    View = glm::rotate(
        View,
        -Pitch,
        glm::vec3{1.0f, 0.0f, 0.0f}
    );

    View = glm::rotate(
        View,
        -Yaw,
        glm::vec3{0.0f, 1.0f, 0.0f}
    );

    View = glm::translate(
        View,
        -Eye
    );

    return View;
}

glm::mat4 Player::ProjectionMatrix(float Aspect) const
{
    return glm::perspective(
        glm::radians(CurrentFov),
        Aspect,
        0.05f,
        76.0f
    );
}

const glm::vec3& Player::Position() const
{
    return PlayerPosition;
}

float Player::Stamina() const
{
    return CurrentStamina;
}

float Player::Fov() const
{
    return CurrentFov;
}
