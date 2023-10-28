#include "controller_device.h"

ControllerDevice::ControllerDevice(vr::ETrackedControllerRole role) : role_(role), device_id_(vr::k_unTrackedDeviceIndexInvalid) {};

vr::EVRInitError ControllerDevice::Activate(uint32_t unObjectId) {
	vr::VRDriverLog()->Log("ControllerDevice::Activate");

	const vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
	vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32, role_);

	vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, "MySampleControllerModel_1");

	vr::VRProperties()->SetStringProperty(container, vr::Prop_InputProfilePath_String,
		"{immersive_gloves_controller}/input/immersive_gloves_controller_profile.json");

	vr::VRDriverInput()->CreateScalarComponent(container, "/input/index/value", &input_handles_[kInputHandle_index_value],
		vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);

	vr::VRDriverInput()->CreateScalarComponent(container, "/input/middle/value", &input_handles_[kInputHandle_middle_value],
		vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);

	vr::VRDriverInput()->CreateScalarComponent(container, "/input/ring/value", &input_handles_[kInputHandle_ring_value],
		vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);

	vr::VRDriverInput()->CreateScalarComponent(container, "/input/pinky/value", &input_handles_[kInputHandle_pinky_value],
		vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);

	device_id_ = unObjectId;
	return vr::VRInitError_None;
}

void ControllerDevice::RunFrame() {
	//vr::VRServerDriverHost()->TrackedDevicePoseUpdated(device_id_, GetPose(), sizeof(vr::DriverPose_t));

	//vr::VRDriverInput()->UpdateScalarComponent(input_handles_[kInputHandle_index_value], 1, 0.0);
	//vr::VRDriverInput()->UpdateScalarComponent(input_handles_[kInputHandle_middle_value], 0.75, 0.0);
	//vr::VRDriverInput()->UpdateScalarComponent(input_handles_[kInputHandle_ring_value], 0.5, 0.0);
	//vr::VRDriverInput()->UpdateScalarComponent(input_handles_[kInputHandle_pinky_value], 0.25, 0.0);
}

void ControllerDevice::HandleEvent(const vr::VREvent_t& vrevent) {
	char logstring[50] = {};
	sprintf_s(logstring, "trackedDeviceIndex %d - vrevent.eventType %d", vrevent.trackedDeviceIndex, vrevent.eventType);
	vr::VRDriverLog()->Log(logstring);

	switch (vrevent.eventType) {
	case vr::VREvent_PropertyChanged: {
		vr::VRDriverLog()->Log("Buzz!");
	}
	//case vr::VREvent_Input_HapticVibration: {
	//	if (vrevent.data.hapticVibration.componentHandle == input_handles_[kInputHandle_haptic]) {
	//		vr::VRDriverLog()->Log("Buzz!");
	//	}
	//	break;
	//}
	}
}

void ControllerDevice::Deactivate() {
}

void ControllerDevice::EnterStandby() {
}

void* ControllerDevice::GetComponent(const char* pchComponentNameAndVersion) {
	return nullptr;
}

void ControllerDevice::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
	if (unResponseBufferSize >= 1)
		pchResponseBuffer[0] = 0;
}

// 3x3 or 3x4 matrix
template < class T >
vr::HmdQuaternion_t HmdQuaternion_FromMatrix(const T& matrix)
{
	vr::HmdQuaternion_t q{};

	q.w = sqrt(fmax(0, 1 + matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2])) / 2;
	q.x = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.y = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.z = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;

	q.x = copysign(q.x, matrix.m[2][1] - matrix.m[1][2]);
	q.y = copysign(q.y, matrix.m[0][2] - matrix.m[2][0]);
	q.z = copysign(q.z, matrix.m[1][0] - matrix.m[0][1]);

	return q;
}

vr::DriverPose_t ControllerDevice::GetPose() {
	vr::DriverPose_t pose = { 0 };

	pose.poseIsValid = true;
	pose.result = vr::TrackingResult_Running_OK;
	pose.deviceIsConnected = true;

	pose.qWorldFromDriverRotation.w = 1.f;
	pose.qDriverFromHeadRotation.w = 1.f;

	pose.qRotation.w = 1.f;

	vr::TrackedDevicePose_t hmd_pose{};
	vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0.f, &hmd_pose, 1);

	const vr::HmdQuaternion_t hmd_orientation = HmdQuaternion_FromMatrix(hmd_pose.mDeviceToAbsoluteTracking);
	pose.qRotation = hmd_orientation;

	pose.vecPosition[0] = role_ == vr::TrackedControllerRole_LeftHand
		? hmd_pose.mDeviceToAbsoluteTracking.m[0][3] - 0.2f
		: hmd_pose.mDeviceToAbsoluteTracking.m[0][3] + 0.2f;

	pose.vecPosition[1] = hmd_pose.mDeviceToAbsoluteTracking.m[1][3];
	pose.vecPosition[2] = hmd_pose.mDeviceToAbsoluteTracking.m[2][3] - 0.5f;

	return pose;
}