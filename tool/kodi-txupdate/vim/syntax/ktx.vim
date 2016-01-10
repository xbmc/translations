" Vim syntax file
" Language: kodi-txupdate.conf
" Maintainer: Attila Jakosa (alanwww1)
" Latest Revision: 01.2016

syn keyword SettingKey set pset tset clear setvar 
syn keyword CreateOption GITCommit SkipVersionBump MajorVersionBump
syn match CommentLine "\v#.*$" 
syn region ExternalVariable1 start="$(" end=")"
syn match CreateResource "\vcreate resource" 
syn keyword InternalVariableUPS UPSOwner UPSRepo UPSBranch UPSLPath UPSAXMLPath UPSLFormInAXML UPSChLogPath
syn keyword InternalVariableLOC LOCOwner LOCRepo LOCBranch LOCLPath LOCAXMLPath LOCLFormInAXML LOCChLogPath
syn keyword InternalVariableMRG LOCOwner MRGLPath MRGAXMLPath MRGChLogPath
syn keyword InternalVariableUPSSRC UPSSRCOwner UPSSRCRepo UPSSRCBranch UPSSRCLPath UPSSRCAXMLPath
syn keyword InternalVariableLOCSRC LOCSRCOwner LOCSRCRepo LOCSRCBranch LOCSRCLPath LOCSRCAXMLPath LOCSRCLFormInAXML LOCSRCChLogPath
syn keyword InternalVariableTRX TRXProjectName TRXLongProjectName TRXResName TRXLForm
syn keyword InternalVariableUPD UPDProjectName UPDLongProjectName UPDResName UPDLForm
syn keyword InternalVariableGeneral ResName ChgLogFormat GitCommitText GitCommitTextSRC MRGLFilesDir UPSLocalPath UPDLFilesDir SupportEmailAddr SRCLCode BaseLForm LTeamLFormat LDatabaseURL MinComplPercent CacheExpire ForceComm ForceGitDloadToCache SkipGitReset Rebrand ForceTXUpd IsLangAddon HasOnlyAddonXML
syn keyword InternalVariablePush GitPushInterval ForceGitPush SkipGitPush
syn keyword Shortvariables $LOC $UPS $UPD $MRG $TRX $UPSSRC $LOCSRC
syn keyword Shortvariables LOC UPS UPD MRG TRX UPSSRC LOCSRC

hi def link SettingKey Keyword
hi def link CommentLine Comment
highlight ExternalVariable1 ctermfg=DarkGreen
highlight CreateResource ctermfg=Magenta
highlight InternalVariableUPS ctermfg=Red
highlight InternalVariableLOC ctermfg=Red
highlight InternalVariableMRG ctermfg=Red
highlight InternalVariableUPSSRC ctermfg=Red
highlight InternalVariableLOCSRC ctermfg=Red
highlight InternalVariableTRX ctermfg=Red
highlight InternalVariableUPD ctermfg=Red
highlight InternalVariableGeneral ctermfg=Red
highlight InternalVariablePush ctermfg=Red
highlight ShortVariables ctermfg=Brown
highlight CreateOption ctermfg=Black ctermbg=Gray

