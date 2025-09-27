#pragma once

#include "atlas.h"
#include "animation.h"

class Player
{
public:
	enum class Facing
	{
		Up,
		Down,
		Left,
		Right
	};

private:
	Animation animationIdleUp;
	Animation animationIdleDown;
	Animation animationIdleLeft;
	Animation animationIdleRight;
	Animation animationRunUp;
	Animation animationRunDown;
	Animation animationRunLeft;
	Animation animationRunRight;
	Animation *currentAnimation = &animationIdleDown;
	Vector2 position;
	Vector2 velocity;
	Vector2 pos_target;
	const float speed = 30.0f;
	Facing facing = Facing::Down;

public:
	Player(Atlas *atlasIdleUp, Atlas *atlasIdleDown, Atlas *atlasIdleLeft, Atlas *atlasIdleRight,
		   Atlas *atlasRunUp, Atlas *atlasRunDown, Atlas *atlasRunLeft, Atlas *atlasRunRight)
	{
		animationIdleUp.setLoop(true);
		animationIdleUp.setInterval(0.1f);
		animationIdleUp.addFrame(atlasIdleUp);

		animationIdleDown.setLoop(true);
		animationIdleDown.setInterval(0.1f);
		animationIdleDown.addFrame(atlasIdleDown);

		animationIdleLeft.setLoop(true);
		animationIdleLeft.setInterval(0.1f);
		animationIdleLeft.addFrame(atlasIdleLeft);

		animationIdleRight.setLoop(true);
		animationIdleRight.setInterval(0.1f);
		animationIdleRight.addFrame(atlasIdleRight);

		animationRunUp.setLoop(true);
		animationRunUp.setInterval(0.1f);
		animationRunUp.addFrame(atlasRunUp);

		animationRunDown.setLoop(true);
		animationRunDown.setInterval(0.1f);
		animationRunDown.addFrame(atlasRunDown);

		animationRunLeft.setLoop(true);
		animationRunLeft.setInterval(0.1f);
		animationRunLeft.addFrame(atlasRunLeft);

		animationRunRight.setLoop(true);
		animationRunRight.setInterval(0.1f);
		animationRunRight.addFrame(atlasRunRight);
	}
	~Player() = default;

	void onUpdate(float delta)
	{
		if (!position.approx(pos_target))
			velocity = (pos_target - position).normalize() * speed;
		else
			velocity = {0, 0};

		if ((pos_target - position).length() <= velocity.length() * delta)
			position = pos_target;
		else
			position += velocity * delta;

		if (velocity.approx({0, 0}))
		{
			switch (facing)
			{
			case Facing::Up:
				currentAnimation = &animationIdleUp;
				break;
			case Facing::Down:
				currentAnimation = &animationIdleDown;
				break;
			case Facing::Left:
				currentAnimation = &animationIdleLeft;
				break;
			case Facing::Right:
				currentAnimation = &animationIdleRight;
				break;
			}
		}
		else
		{
			if (std::abs(velocity.y) >= 0.0001f && std::abs(velocity.y) > std::abs(velocity.x))
				facing = velocity.y > 0 ? Facing::Down : Facing::Up;
			else if (std::abs(velocity.x) >= 0.0001f)
				facing = velocity.x > 0 ? Facing::Right : Facing::Left;

			switch (facing)
			{
			case Facing::Up:
				currentAnimation = &animationRunUp;
				break;
			case Facing::Down:
				currentAnimation = &animationRunDown;
				break;
			case Facing::Left:
				currentAnimation = &animationRunLeft;
				break;
			case Facing::Right:
				currentAnimation = &animationRunRight;
				break;
			}
		}
		if (currentAnimation)
		{
			currentAnimation->setPosition(position);
			currentAnimation->onUpdate(delta);
		}
	}
	void onRender(const Camera &camera)
	{
		if (currentAnimation)
			currentAnimation->onRender(camera);
	}
	void setPosition(const Vector2 &position)
	{
		this->position = position;
		this->pos_target = position;
	}
	const Vector2 &getPosition() const
	{
		return position;
	}
	void setTarget(const Vector2 &pos_target)
	{
		this->pos_target = pos_target;
	}
};