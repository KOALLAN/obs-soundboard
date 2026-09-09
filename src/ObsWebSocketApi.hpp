#pragma once

// Compatible subset of obsproject/obs-websocket's GPL-2.0-or-later public API.
// Original authors: Stephane Lepin and Kyle Manning.

#include <obs.h>

#include <cassert>
#include <cstring>

// Minimal subset of obs-websocket's public vendor API. Keeping the bridge here
// avoids a runtime dependency: every supported OBS version already ships
// obs-websocket and exposes these procedures through libobs.
using obs_websocket_vendor = void *;
using obs_websocket_request_callback_function = void (*)(obs_data_t *, obs_data_t *, void *);

struct obs_websocket_request_callback {
	obs_websocket_request_callback_function callback;
	void *priv_data;
};

namespace ObsWebSocketApi {
inline proc_handler_t *&procHandler()
{
	static proc_handler_t *handler = nullptr;
	return handler;
}

inline proc_handler_t *getProcHandler()
{
	proc_handler_t *globalHandler = obs_get_proc_handler();
	assert(globalHandler != nullptr);

	calldata_t data = {};
	if (!proc_handler_call(globalHandler, "obs_websocket_api_get_ph", &data)) {
		calldata_free(&data);
		return nullptr;
	}

	auto *handler = static_cast<proc_handler_t *>(calldata_ptr(&data, "ph"));
	calldata_free(&data);
	return handler;
}

inline bool ensureProcHandler()
{
	if (!procHandler())
		procHandler() = getProcHandler();
	return procHandler() != nullptr;
}

inline obs_websocket_vendor registerVendor(const char *vendorName)
{
	if (!ensureProcHandler() || !vendorName || !std::strlen(vendorName))
		return nullptr;

	calldata_t data = {};
	calldata_set_string(&data, "name", vendorName);
	proc_handler_call(procHandler(), "vendor_register", &data);
	auto vendor = calldata_ptr(&data, "vendor");
	calldata_free(&data);
	return vendor;
}

inline bool registerRequest(obs_websocket_vendor vendor, const char *requestType,
			    obs_websocket_request_callback_function callback, void *privateData)
{
	if (!ensureProcHandler() || !vendor || !requestType || !std::strlen(requestType) || !callback)
		return false;

	obs_websocket_request_callback request = {callback, privateData};
	calldata_t data = {};
	calldata_set_ptr(&data, "vendor", vendor);
	calldata_set_string(&data, "type", requestType);
	calldata_set_ptr(&data, "callback", &request);
	proc_handler_call(procHandler(), "vendor_request_register", &data);
	const bool success = calldata_bool(&data, "success");
	calldata_free(&data);
	return success;
}

inline bool emitEvent(obs_websocket_vendor vendor, const char *eventType, obs_data_t *eventData)
{
	if (!ensureProcHandler() || !vendor || !eventType || !std::strlen(eventType) || !eventData)
		return false;

	calldata_t data = {};
	calldata_set_ptr(&data, "vendor", vendor);
	calldata_set_string(&data, "type", eventType);
	calldata_set_ptr(&data, "data", eventData);
	proc_handler_call(procHandler(), "vendor_event_emit", &data);
	const bool success = calldata_bool(&data, "success");
	calldata_free(&data);
	return success;
}
} // namespace ObsWebSocketApi
