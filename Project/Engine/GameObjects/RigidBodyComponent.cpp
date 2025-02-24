#include "RigidBodyComponent.h"
//#include "GameObject.h"
//#include <glm/gtc/quaternion.hpp>
//#include <glm/gtx/euler_angles.hpp>
//#include <glm/gtc/constants.hpp>
//
//void RigidBodyComponent::update() {
//    auto parent = getParent();
//    if (parent && m_body) {
//        // Get the current pose from the PhysX body.
//        PxTransform pose = m_body->getGlobalPose();
//
//        // Update position.
//        parent->getTransform()->position = glm::vec3(pose.p.x, pose.p.y, pose.p.z);
//
//        // Convert the PhysX quaternion to a glm quaternion.
//        glm::quat quat(pose.q.w, pose.q.x, pose.q.y, pose.q.z);
//        // Convert the quaternion to Euler angles (in radians) and then to degrees.
//        glm::vec3 eulerRadians = glm::eulerAngles(quat);
//        parent->getTransform()->rotation = glm::degrees(eulerRadians);
//
//       
//    }
//}
