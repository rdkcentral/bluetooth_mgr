#include "json_parse.h"


typedef enum {
    eMD5Sum,
    eRdkSsaCli,
    eMfrUtil,
    eWpeFrameworkSecurityUtility
#ifdef GETRDMMANIFESTVERSION_IN_SCRIPT
    ,eGetInstalledRdmManifestVersion
#endif
#ifdef GETMODEL_IN_SCRIPT
    ,eGetModelNum
#endif
} SYSCMD;

#define DEFAULT_DL_ALLOC    1024

int MemDLAlloc(DownloadData *pDwnData, size_t szDataSize);
size_t RunCommand(SYSCMD eSysCmd, const char *pArgs, char *pResult, size_t szResultSize);
int getJsonRpc(char *post_data, DownloadData* pJsonRpc);
int getJRPCTokenData(char *token, char *pJsonStr, unsigned int token_size);
