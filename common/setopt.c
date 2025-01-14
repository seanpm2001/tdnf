/*
 * Copyright (C) 2015-2022 VMware, Inc. All Rights Reserved.
 *
 * Licensed under the GNU General Public License v2 (the "License");
 * you may not use this file except in compliance with the License. The terms
 * of the License are located in the COPYING file of this distribution.
 */

#include "includes.h"

void
TDNFFreeCmdOpt(
    PTDNF_CMD_OPT pCmdOpt
    );

uint32_t
AddSetOpt(
    PTDNF_CMD_ARGS pCmdArgs,
    const char* pszOptArg
    )
{
    uint32_t dwError = 0;
    PTDNF_CMD_OPT pCmdOpt = NULL;

    if(!pCmdArgs || IsNullOrEmptyString(pszOptArg))
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }
    dwError = GetOptionAndValue(pszOptArg, &pCmdOpt);
    BAIL_ON_TDNF_ERROR(dwError);

    if(!strcmp(pCmdOpt->pszOptName, "tdnf.conf"))
    {
        TDNF_SAFE_FREE_MEMORY(pCmdArgs->pszConfFile);
        dwError = TDNFAllocateString(
                      pCmdOpt->pszOptValue,
                      &pCmdArgs->pszConfFile);
        BAIL_ON_TDNF_ERROR(dwError);
    }

    dwError = AddSetOptWithValues(pCmdArgs, pCmdOpt->pszOptName, pCmdOpt->pszOptValue);
    BAIL_ON_TDNF_ERROR(dwError);

cleanup:
    if(pCmdOpt)
    {
        TDNFFreeCmdOpt(pCmdOpt);
    }
    return dwError;

error:
    TDNF_SAFE_FREE_MEMORY(pCmdArgs->pszConfFile);
    goto cleanup;
}

uint32_t
AddSetOptWithValues(
    PTDNF_CMD_ARGS pCmdArgs,
    const char *pszOptArg,
    const char *pszOptValue
    )
{
    uint32_t dwError = 0;
    PTDNF_CMD_OPT pCmdOpt = NULL;
    PTDNF_CMD_OPT pSetOptTemp = NULL;

    if(!pCmdArgs ||
       IsNullOrEmptyString(pszOptArg))
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    dwError = TDNFAllocateMemory(1, sizeof(TDNF_CMD_OPT), (void **)&pCmdOpt);
    BAIL_ON_TDNF_ERROR(dwError);

    dwError = TDNFAllocateString(pszOptArg, &pCmdOpt->pszOptName);
    BAIL_ON_TDNF_ERROR(dwError);

    if (pszOptValue)
    {
        dwError = TDNFAllocateString(pszOptValue, &pCmdOpt->pszOptValue);
        BAIL_ON_TDNF_ERROR(dwError);
    }

    pSetOptTemp = pCmdArgs->pSetOpt;
    if (pSetOptTemp)
    {
        while (pSetOptTemp->pNext)
        {
            pSetOptTemp = pSetOptTemp->pNext;
        }
        pSetOptTemp->pNext = pCmdOpt;
    }
    else
    {
        pCmdArgs->pSetOpt = pCmdOpt;
    }

cleanup:
    return dwError;

error:
    if (pCmdOpt)
    {
        TDNFFreeCmdOpt(pCmdOpt);
    }
    goto cleanup;
}

uint32_t
GetOptionAndValue(
    const char* pszOptArg,
    PTDNF_CMD_OPT* ppCmdOpt
    )
{
    uint32_t dwError = 0;
    const char* EQUAL_SIGN = "=";
    const char* pszIndex = NULL;
    PTDNF_CMD_OPT pCmdOpt = NULL;
    int nEqualsPos = -1;

    if(IsNullOrEmptyString(pszOptArg) || !ppCmdOpt)
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    pszIndex = strstr(pszOptArg, EQUAL_SIGN);
    if(!pszIndex)
    {
        dwError = ERROR_TDNF_SETOPT_NO_EQUALS;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    dwError = TDNFAllocateMemory(1, sizeof(TDNF_CMD_OPT), (void**)&pCmdOpt);
    BAIL_ON_TDNF_ERROR(dwError);

    dwError = TDNFAllocateString(pszOptArg, &pCmdOpt->pszOptName);
    BAIL_ON_TDNF_ERROR(dwError);

    nEqualsPos = pszIndex - pszOptArg;
    pCmdOpt->pszOptName[nEqualsPos] = '\0';

    dwError = TDNFAllocateString(pszOptArg+nEqualsPos+1,
                                 &pCmdOpt->pszOptValue);
    BAIL_ON_TDNF_ERROR(dwError);

    *ppCmdOpt = pCmdOpt;
cleanup:
    return dwError;

error:
    if(ppCmdOpt)
    {
        *ppCmdOpt = NULL;
    }
    if(pCmdOpt)
    {
        TDNFFreeCmdOpt(pCmdOpt);
    }
    goto cleanup;
}

static
uint32_t
_TDNFGetCmdOpt(
    PTDNF_CMD_ARGS pArgs,
    const char *pszOptName,
    PTDNF_CMD_OPT *ppOpt
    )
{
    uint32_t dwError = 0;
    PTDNF_CMD_OPT pOpt = NULL;
    int nHasOpt = 0;

    for (pOpt = pArgs->pSetOpt;
         pOpt;
         pOpt = pOpt->pNext)
    {
        if (!strcmp(pOpt->pszOptName, pszOptName))
        {
            nHasOpt = 1;
            break;
        }
    }

    if (!nHasOpt)
    {
        dwError = ERROR_TDNF_OPT_NOT_FOUND;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    *ppOpt = pOpt;
error:
    return dwError;
}

/*
 * check if option is present.
 * *pnHasOpt = 0 if option not preset
 * *pnHasOpt = 1 if option is present
*/
uint32_t
TDNFHasOpt(
    PTDNF_CMD_ARGS pArgs,
    const char *pszOptName,
    int *pnHasOpt
    )
{
    uint32_t dwError = 0;
    PTDNF_CMD_OPT pOpt = NULL;
    int nHasOpt = 0;

    if(!pArgs || !pArgs->pSetOpt ||
       IsNullOrEmptyString(pszOptName) || !pnHasOpt)
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    dwError = _TDNFGetCmdOpt(pArgs, pszOptName, &pOpt);
    BAIL_ON_TDNF_ERROR(dwError);

    nHasOpt = 1;

    *pnHasOpt = nHasOpt;

cleanup:
    return dwError;

error:
    if (dwError == ERROR_TDNF_OPT_NOT_FOUND)
    {
        dwError = 0;
    }
    goto cleanup;
}

uint32_t
TDNFGetCmdOptValue(
    PTDNF_CMD_ARGS pArgs,
    const char *pszOptName,
    char **ppszOptValue
    )
{
    uint32_t dwError = 0;
    PTDNF_CMD_OPT pOpt = NULL;
    char *pszOptValue = NULL;

    if(!pArgs || !pArgs->pSetOpt ||
       IsNullOrEmptyString(pszOptName) || !ppszOptValue)
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    dwError = _TDNFGetCmdOpt(pArgs, pszOptName, &pOpt);
    BAIL_ON_TDNF_ERROR(dwError);

    if (pOpt->pszOptValue)
    {
        dwError = TDNFAllocateString(pOpt->pszOptValue, &pszOptValue);
        BAIL_ON_TDNF_ERROR(dwError);
    }

    *ppszOptValue = pszOptValue;

cleanup:
    return dwError;

error:
    TDNF_SAFE_FREE_MEMORY(pszOptValue);
    goto cleanup;
}

/*
 * if get fails, use the default value (if supplied)
*/
uint32_t
TDNFGetOptWithDefault(
    PTDNF_CMD_ARGS pArgs,
    const char *pszOptName,
    const char *pszDefault,
    char **ppszOptValue
    )
{
    uint32_t dwError = 0;
    char *pszOptValue = NULL;

    dwError = TDNFGetCmdOptValue(pArgs, pszOptName, &pszOptValue);
    if (!IsNullOrEmptyString(pszDefault) &&
        dwError == ERROR_TDNF_OPT_NOT_FOUND)
    {
        dwError = TDNFAllocateString(pszDefault, &pszOptValue);
    }
    BAIL_ON_TDNF_ERROR(dwError);

    *ppszOptValue = pszOptValue;
error:
    return dwError;
}

uint32_t
TDNFSetOpt(
    PTDNF_CMD_ARGS pArgs,
    const char *pszOptName,
    const char *pszOptValue
    )
{
    uint32_t dwError = 0;
    PTDNF_CMD_OPT pOpt = NULL;

    if (!pArgs || IsNullOrEmptyString(pszOptName) ||
        IsNullOrEmptyString(pszOptValue))
    {
        dwError = ERROR_TDNF_INVALID_PARAMETER;
        BAIL_ON_TDNF_ERROR(dwError);
    }

    dwError = _TDNFGetCmdOpt(pArgs, pszOptName, &pOpt);
    if (dwError == ERROR_TDNF_OPT_NOT_FOUND)
    {
        dwError = 0;
    }
    BAIL_ON_TDNF_ERROR(dwError);

    if (pOpt)
    {
        TDNF_SAFE_FREE_MEMORY(pOpt->pszOptValue);

        dwError = TDNFAllocateString(pszOptValue, &pOpt->pszOptValue);
        BAIL_ON_TDNF_ERROR(dwError);
    }
    else
    {
        dwError = AddSetOptWithValues(pArgs, pszOptName, pszOptValue);
        BAIL_ON_TDNF_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}
