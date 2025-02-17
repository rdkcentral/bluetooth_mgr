#ifdef BTR_SYS_DIAG_IARM_ENABLE

#include <stdbool.h>
#include <stdio.h>
#include "rdk_fwdl_utils.h"
#include <string.h>
#include <curl/curl.h> // for getJsonRpc (assuming it uses curl)
#include "json_parse.h"
#include "downloadUtil.h"
#include <secure_wrapper.h>
#include "btrMgr_DeviceUtils.h"
#include "btrMgr_logger.h"

#define WPEFRAMEWORKSECURITYUTILITY "/usr/bin/WPEFrameworkSecurityUtility"
#define MFRUTIL                         "/usr/bin/mfr_util %s" // --PDRIVersion"
#define GETINSTALLEDRDMMANIFESTVERSIONSCRIPT  "/lib/rdk/cdlSupport.sh getInstalledRdmManifestVersion"
#define RDKSSACLI               "/usr/bin/rdkssacli %s" // \"{STOR=GET,SRC=CREDREFERENCE,DST=STDOUT}\"";
#define MD5SUM                 "/usr/bin/md5sum %s"
#define GETMODELSCRIPT    "/lib/rdk/cdlSupport.sh getModel"
int MemDLAlloc( DownloadData *pDwnData, size_t szDataSize )
{
    void *ptr;
    int iRet = 1;

    if( pDwnData != NULL )
    {
        pDwnData->datasize = 0;
        ptr = malloc( szDataSize );
        pDwnData->pvOut = ptr;
        pDwnData->datasize = 0;
        if( ptr != NULL )
        {
            pDwnData->memsize = szDataSize;
            *(char *)ptr = 0;
            iRet = 0;
        }
        else
        {
            pDwnData->memsize = 0;
            BTRMGRLOG_ERROR("MemDLAlloc: Failed to allocate memory for XCONF download\n");
        }
    }
    return iRet;
}

int getJsonRpc(char *post_data, DownloadData* pJsonRpc )
{
    void *Curl_req = NULL;
    char token[256];
    char jsondata[256];
    int httpCode = 0;
    FileDwnl_t req_data;
    int curl_ret_code = -1;
    char header[]  = "Content-Type: application/json";
    char token_header[300];

    *token = 0;
    *jsondata = 0;
    RunCommand( eWpeFrameworkSecurityUtility, NULL, jsondata, sizeof(jsondata) );

    getJRPCTokenData(token, jsondata, sizeof(token));
    if (pJsonRpc->pvOut != NULL) {
        req_data.pHeaderData = header;
        req_data.pDlHeaderData = NULL;
        snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);
        req_data.pPostFields = post_data;
        req_data.pDlData = pJsonRpc;
        snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");
        Curl_req = doCurlInit();
        if (Curl_req != NULL) {
            curl_ret_code = getJsonRpcData(Curl_req, &req_data, token_header, &httpCode );

            doStopDownload(Curl_req);
        }else {
            BTRMGRLOG_ERROR("%s: doCurlInit fail\n", __FUNCTION__);
        }
    }else {
        BTRMGRLOG_DEBUG("%s: Failed to allocate memory using malloc\n", __FUNCTION__);
    }
    return curl_ret_code;
}

size_t RunCommand( SYSCMD eSysCmd, const char *pArgs, char *pResult, size_t szResultSize )
{
    FILE *fp;
    size_t nbytes_read = 0;

    if( pResult != NULL && szResultSize >= 1 )
    {
        *pResult = 0;
        switch( eSysCmd )
        {
           case eMD5Sum :
               if( pArgs != NULL )
               {
                   fp = v_secure_popen( "r", MD5SUM, pArgs );
               }
               else
               {
                   fp = NULL;
                   BTRMGRLOG_ERROR("RunCommand: Error, %s requires an input argument\n",MD5SUM);
               }
               break;

           case eRdkSsaCli :
               if( pArgs != NULL )
               {
                   fp = v_secure_popen( "r", RDKSSACLI, pArgs );
               }
               else
               {
                   fp = NULL;
                   BTRMGRLOG_ERROR("RunCommand: Error, %s requires an input argument\n",RDKSSACLI);
               }
               break;

           case eMfrUtil :
               if( pArgs != NULL )
               {
                   fp = v_secure_popen( "r", MFRUTIL, pArgs );
               }
               else
               {
                   fp = NULL;
                   BTRMGRLOG_ERROR("RunCommand: Error, %s requires an input argument\n",MFRUTIL);
				    }
               break;

           case eWpeFrameworkSecurityUtility :
               fp = v_secure_popen( "r", WPEFRAMEWORKSECURITYUTILITY );
               break;


#ifdef GETRDMMANIFESTVERSION_IN_SCRIPT
           case eGetInstalledRdmManifestVersion :
               fp = v_secure_popen( "r", GETINSTALLEDRDMMANIFESTVERSIONSCRIPT );
               break;
#endif
#ifdef GETMODEL_IN_SCRIPT
           case eGetModelNum :
               fp = v_secure_popen( "r", GETMODELSCRIPT );
               break;
#endif

           default:
               fp = NULL;
               BTRMGRLOG_ERROR("RunCommand: Error, unknown request type %d\n", (int)eSysCmd);
               break;
        }

        if( fp != NULL )
        {
            nbytes_read = fread( pResult, 1, szResultSize - 1, fp );
            v_secure_pclose( fp );
            if( nbytes_read != 0 )
            {
                BTRMGRLOG_INFO("%s: Successful read %zu bytes\n", __FUNCTION__, nbytes_read);
                pResult[nbytes_read] = '\0';
                nbytes_read = strnlen( pResult, szResultSize ); // fread might include NULL characters, get accurate count
            }
            else
            {
                BTRMGRLOG_ERROR("%s fread fails:%d\n", __FUNCTION__, nbytes_read);
            }
        }
        else
        {
            BTRMGRLOG_ERROR("RunCommand: Failed to open pipe command execution\n");
        }
    }
    else
    {
        BTRMGRLOG_ERROR("RunCommand: Error, input argument invalid\n");
    }
    return nbytes_read;
}


int getJRPCTokenData( char *token, char *pJsonStr, unsigned int token_size )
{
    JSON *pJson = NULL;
    char status[8];
    int ret = -1;

    if (token == NULL || pJsonStr == NULL) {
        BTRMGRLOG_INFO( "%s: Parameter is NULL\n", __FUNCTION__);
        return ret;
    }
    *status = 0;
    pJson = ParseJsonStr( pJsonStr );
    if( pJson != NULL )
    {
        GetJsonVal(pJson, "token", token, token_size);
        GetJsonVal(pJson, "success", status, sizeof(status));
        BTRMGRLOG_INFO( "%s: status:%s\n", __FUNCTION__, status);
        FreeJson( pJson );
        ret = 0;
    }
    return ret;
}
#endif