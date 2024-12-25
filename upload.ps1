# Read argument from command line
param(
    [string]$COM_PORT
)

$CODE_VERSION=git rev-list HEAD --first-parent --count
Set-Content -Path .\main\version.h @"
#ifndef CARCOMPUTER_VERSION_H
#define CARCOMPUTER_VERSION_H

#define APP_VERSION "$CODE_VERSION"

#endif //CARCOMPUTER_VERSION_H
"@
idf.py flash -p $COM_PORT
if ($?) {
    idf.py monitor -p $COM_PORT
}