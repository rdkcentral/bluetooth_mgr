# Threading Changes for btrMgr_IncomingConnectionAuthentication

## Problem Statement
The `btrMgr_IncomingConnectionAuthentication` function was blocking the device status callback (`btrMgr_DeviceStatusCb`) while waiting for user response from the UI. This could delay processing of other device status events.

## Solution
Implemented a threaded version of the authentication function that:
1. Spawns a detached pthread to handle authentication
2. Returns immediately from the device status callback
3. Handles authentication and cleanup in the background thread

## Changes Made

### 1. Added pthread support
- Added `#include <pthread.h>` to the includes

### 2. Created thread context structure
```c
typedef struct {
    stBTRCoreDevStatusCBInfo statusCB;
    int auth;
} btrMgr_AuthThreadContext_t;
```

### 3. Implemented threaded functions
- `btrMgr_IncomingConnectionAuthenticationThread()` - Thread function that calls the original authentication
- `btrMgr_IncomingConnectionAuthenticationAsync()` - Wrapper that spawns the thread

### 4. Updated call sites
Modified three locations in `btrMgr_DeviceStatusCb` to use the async version:
- Line ~9657: AUTO_CONNECT_ENABLED path for LE HID gamepads
- Line ~9742: When device transitions from Disconnected to Connected
- Line ~9755: When device transitions from Connecting to Connected

**Note:** The call in `BTRMGR_ConnectGamepads_StartUp` was kept synchronous as it's initialization code and not in a callback.

## Technical Details

### Thread Management
- Uses detached threads (`PTHREAD_CREATE_DETACHED`) to avoid need for explicit joining
- Thread cleans up its own allocated context when done
- Thread function handles memory deallocation with `free(threadCtx)`

### Memory Safety
- Deep copies `stBTRCoreDevStatusCBInfo` using `memcpy` (safe since structure contains only value types)
- Allocates thread context on heap, freed by thread when complete
- Proper error handling for allocation failures

### Authentication Result Handling
The authentication result is handled within the thread by `btrMgr_IncomingConnectionAuthentication`, which:
- Disconnects the device if authentication fails (auth == 0)
- Allows connection to proceed if authentication succeeds (auth == 1)

## Testing
- Code review completed and all issues addressed
- CodeQL security scan passed with no issues
- Changes are minimal and surgical, preserving existing behavior

## Files Modified
- `src/ifce/btrMgr.c` - Main implementation file
  - Added: 75 lines
  - Removed: 13 lines
  - Net change: +62 lines

## Patch File
The complete changes are available in `btrMgr_IncomingConnectionAuthentication_threading.patch`

## Application
To apply this patch:
```bash
git apply btrMgr_IncomingConnectionAuthentication_threading.patch
```

Or to apply as commits:
```bash
git am btrMgr_IncomingConnectionAuthentication_threading.patch
```
