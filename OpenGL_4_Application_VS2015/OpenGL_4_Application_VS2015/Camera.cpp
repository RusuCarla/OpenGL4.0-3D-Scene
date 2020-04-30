//
//  Camera.cpp
//  Lab5
//
//  Created by CGIS on 28/10/2016.
//  Copyright © 2016 CGIS. All rights reserved.
//

#include "Camera.hpp"

namespace gps {
	float lastX, lastY, yaw, pitch;
	int firstMouse = 1;
	Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget)
	{
		this->cameraPosition = cameraPosition;
		this->cameraTarget = cameraTarget;
		this->cameraDirection = glm::normalize(cameraTarget - cameraPosition);
		this->cameraRightDirection = glm::normalize(glm::cross(this->cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
	}

	glm::mat4 Camera::getViewMatrix()
	{
		return glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 Camera::getCameraTarget()
	{
		return cameraTarget;
	}

	void Camera::setCameraPos(glm::vec3 pos)
	{
		cameraPosition = pos;
	}

	void Camera::setCameraDir(glm::vec3 dir)
	{
		cameraDirection = dir;
	}

	glm::vec3 Camera::getCameraPos()
	{
		return cameraPosition;
	}

	glm::vec3 Camera::getCameraDir()
	{
		return cameraDirection;
	}

	glm::vec3 Camera::getCameraLeftDir()
	{
		return glm::normalize(glm::cross(this->cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
	}

	void Camera::move(MOVE_DIRECTION direction, float speed)
	{
		printf("%f   %f   %f\n", cameraPosition.x, cameraPosition.y, cameraPosition.z);
		switch (direction) {
		case MOVE_FORWARD:
			cameraPosition += cameraDirection * speed;
			break;

		case MOVE_BACKWARD:
			cameraPosition -= cameraDirection * speed;
			break;

		case MOVE_RIGHT:
			cameraPosition += glm::normalize(glm::cross(this->cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * speed;
			break;

		case MOVE_LEFT:
			cameraPosition -= glm::normalize(glm::cross(this->cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * speed;
			break;

		case MOVE_UP:
			cameraPosition += glm::vec3(0, 1, 0) * speed;
			break;

		case MOVE_DOWN:
			cameraPosition -= glm::vec3(0, 1, 0) * speed;
			break;
		}
	}

	void Camera::rotate(float pitch, float yaw)
	{
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraDirection = glm::normalize(front);
	}

	void Camera::mouse_callback(float xpos, float ypos) {

		if (firstMouse == 1)
		{
			lastX = 320;
			lastY = 240;
			firstMouse = 0;
		}
		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		float sensitivity = 0.2;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		rotate(pitch, yaw);
	}
}
