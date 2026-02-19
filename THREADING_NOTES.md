# Threading Changes for btrMgr_IncomingConnectionAuthentication

## Problem Statement
The `btrMgr_IncomingConnectionAuthentication` function was blocking the device status callback for up to 20 seconds while waiting for UI response, preventing other device status events from being processed.

## Solution
Created a new async version `btrMgr_IncomingConnectionAuthenticationAsync` that:
1. Launches a separate thread to handle authentication
2. Returns immediately without blocking the callback
3. Allows the device status callback to continue processing other events

## Changes Made
1. Added `BTRMGR_AuthenticationThreadData_t` structure to pass device status info to thread
2. Implemented `btrMgr_IncomingConnectionAuthenticationThread` as the thread function
3. Implemented `btrMgr_IncomingConnectionAuthenticationAsync` to launch the thread
4. Updated three call sites in `btrMgr_DeviceStatusCb` to use the async version
5. Kept original `btrMgr_IncomingConnectionAuthentication` for synchronous startup case

## Thread Safety Considerations
- The implementation maintains the same thread safety characteristics as the original code
- Global variables (gEventRespReceived, gAcceptConnection, gListOfPairedDevices) are accessed
- These were already accessed in the original blocking implementation
- For production hardening, consider adding mutex protection and per-device response tracking

## Testing
- Unit tests for the synchronous function remain unchanged and valid
- The async function launches threads and returns immediately (harder to unit test)
- Manual testing recommended to verify device authentication flow

## Files Modified
- src/ifce/btrMgr.c: All changes in this single file

## Patch File
- btrmgr_threading.patch: Git diff patch file for the changes
