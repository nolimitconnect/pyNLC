param(
    [string]$AdbPath = 'F:/Android/Sdk/platform-tools/adb.exe',
    [string]$PackageActivity = 'org.nolimitconnect.nolimitconnect/org.qtproject.qt.android.bindings.QtActivity',
    [int]$LldbPort = 5039,
    [int]$DebugSocketWaitSeconds = 15,
    [int]$DeviceWaitSeconds = 45,
    [string]$WorkspaceFolder = '',
    [switch]$WaitForDebugger,
    [switch]$StopAfterLaunch,
    [switch]$AttachOnly
)

function Resolve-LldbServerHostPath {
    $candidates = @(
        "F:/Android/Sdk/ndk/27.2.12479018/toolchains/llvm/prebuilt/windows-x86_64/lib/clang/18/lib/linux/aarch64/lldb-server",
        "F:/Android/Sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/windows-x86_64/lib/clang/17/lib/linux/aarch64/lldb-server",
        "F:/Android/Sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/windows-x86_64/lib/clang/17.0.2/lib/linux/aarch64/lldb-server"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

$ErrorActionPreference = 'Stop'

function Write-LldbModuleLoadCommands {
    param(
        [string]$Workspace,
        [string]$BuildSubdir,
        [string]$ModuleFileName,
        [string]$ModuleLoadAddress,
        [string]$ModulePreferredBase
    )

    if (-not $Workspace) {
        return
    }

    $commandDir = Join-Path $Workspace $BuildSubdir
    if (-not (Test-Path $commandDir -PathType Container)) {
        New-Item -ItemType Directory -Path $commandDir -Force | Out-Null
    }

    $commandFile = Join-Path $commandDir 'android-lldb-module-load.lldb'
    if ($ModuleLoadAddress -and $ModulePreferredBase) {
        $modulePath = (Join-Path $commandDir $ModuleFileName).Replace('\', '/')
        $runtimeBase = [System.Numerics.BigInteger]::Parse($ModuleLoadAddress, [System.Globalization.NumberStyles]::AllowHexSpecifier)
        $preferredBase = [System.Numerics.BigInteger]::Parse($ModulePreferredBase, [System.Globalization.NumberStyles]::AllowHexSpecifier)
        $slide = $runtimeBase - $preferredBase
        $slideHex = $slide.ToString('x')
        $commands = @(
            ('target modules load --file "{0}" --slide 0x{1}' -f $modulePath, $slideHex),
            ('settings append target.exec-search-paths {0}' -f $commandDir.Replace('\', '/'))
        )
        Set-Content -Path $commandFile -Value $commands -Encoding Ascii
        Write-Host ("Wrote LLDB module load commands to: {0} (runtime 0x{1}, preferred 0x{2}, slide 0x{3})" -f $commandFile, $ModuleLoadAddress, $ModulePreferredBase, $slideHex)
    } else {
        Set-Content -Path $commandFile -Value '# runtime module load address unavailable' -Encoding Ascii
        Write-Host ("Wrote empty LLDB module load commands to: {0}" -f $commandFile)
    }
}

function Get-ModuleLoadAddress {
    param(
        [string]$Adb,
        [string]$DeviceSerial,
        [string]$Pkg,
        [string]$AppPid,
        [string]$ModuleFileName
    )

    if (-not $AppPid) {
        return $null
    }

    $mapsOutput = & $Adb -s $DeviceSerial shell "run-as $Pkg sh -c 'cat /proc/$AppPid/maps | grep $ModuleFileName'" 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $mapsOutput) {
        return $null
    }

    foreach ($line in $mapsOutput) {
        if ($line -notmatch [regex]::Escape($ModuleFileName)) {
            continue
        }

        if ($line -match '^([0-9a-fA-F]+)-[0-9a-fA-F]+') {
            return $Matches[1].ToLowerInvariant()
        }
    }

    return $null
}

function Get-ModulePreferredBase {
    param(
        [string]$Workspace,
        [string]$BuildSubdir,
        [string]$ModuleFileName
    )

    if (-not $Workspace) {
        return $null
    }

    $modulePath = Join-Path (Join-Path $Workspace $BuildSubdir) $ModuleFileName
    if (-not (Test-Path $modulePath -PathType Leaf)) {
        return $null
    }

    $readobjCandidates = @(
        'F:/Android/Sdk/ndk/27.2.12479018/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-readobj.exe',
        'F:/Android/Sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-readobj.exe'
    )

    $readobj = $readobjCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $readobj) {
        return $null
    }

    $headers = & $readobj --program-headers $modulePath 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $headers) {
        return $null
    }

    $collectLoad = $false
    foreach ($line in $headers) {
        if ($line -match '^\s*Type:\s*PT_LOAD\b') {
            $collectLoad = $true
            continue
        }

        if ($collectLoad -and $line -match '^\s*VirtualAddress:\s*0x([0-9A-Fa-f]+)\s*$') {
            return $Matches[1].ToLowerInvariant()
        }

        if ($collectLoad -and $line -match '^\s*Type:\s*') {
            $collectLoad = $false
        }
    }

    return '0'
}

function Test-LldbServerForwardReady {
    param(
        [int]$Port,
        [int]$TimeoutMs = 2000
    )

    $client = New-Object System.Net.Sockets.TcpClient
    try {
        $asyncResult = $client.BeginConnect('127.0.0.1', $Port, $null, $null)
        if (-not $asyncResult.AsyncWaitHandle.WaitOne($TimeoutMs, $false)) {
            return $false
        }

        $client.EndConnect($asyncResult)
        return $client.Connected
    } catch {
        return $false
    } finally {
        $client.Close()
    }
}

if ($WorkspaceFolder) {
    $resolvedWorkspace = [System.IO.Path]::GetFullPath($WorkspaceFolder)
    Write-Host ("Workspace folder (resolved): {0}" -f $resolvedWorkspace)
    if (-not (Test-Path $resolvedWorkspace -PathType Container)) {
        throw ("Workspace folder is missing or inaccessible: {0}" -f $resolvedWorkspace)
    }
}

if (-not (Test-Path $AdbPath)) {
    throw "adb not found at path: $AdbPath"
}

& $AdbPath start-server | Out-Null

function Get-DeviceState {
    param(
        [string]$Adb,
        [string]$Serial
    )

    if (-not $Serial) {
        return $null
    }

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $state = & $Adb -s $Serial get-state 2>$null
    $stateExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference

    if ($stateExitCode -ne 0 -or -not $state) {
        return $null
    }

    return $state.Trim()
}

function Get-ConnectedDeviceSerials {
    param(
        [string]$Adb
    )

    $lines = (& $Adb devices) |
        Select-Object -Skip 1 |
        Where-Object { $_ -match '\S' }

    $serials = @()
    foreach ($line in $lines) {
        if ($line -match '^(\S+)\s+device$') {
            $serials += $Matches[1]
        }
    }

    return $serials
}

function Get-FirstConnectedDeviceSerial {
    param(
        [string]$Adb
    )

    $lines = (& $Adb devices) |
        Select-Object -Skip 1 |
        Where-Object { $_ -match '\S' }

    foreach ($line in $lines) {
        if ($line -match '^(\S+)\s+device$') {
            return $Matches[1]
        }
    }

    return $null
}

function Wait-ForDeviceState {
    param(
        [string]$Adb,
        [string]$Serial,
        [int]$TimeoutSeconds
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $state = Get-DeviceState -Adb $Adb -Serial $Serial
        if ($state -eq 'device') {
            return $true
        }

        if ($state) {
            Write-Host ("Waiting for Android device {0}: current state '{1}'" -f $Serial, $state)
        } else {
            Write-Host ("Waiting for Android device {0}: current state unavailable" -f $Serial)
        }

        Start-Sleep -Seconds 1
    }

    return $false
}

$serial = $null
if ($env:ANDROID_SERIAL) {
    $configuredSerial = $env:ANDROID_SERIAL.Trim()
    if ((Get-DeviceState -Adb $AdbPath -Serial $configuredSerial) -eq 'device') {
        $serial = $configuredSerial
        Write-Host ("Using configured ANDROID_SERIAL: {0}" -f $serial)
    } else {
        $serial = Get-FirstConnectedDeviceSerial -Adb $AdbPath
        if ($serial) {
        Write-Host ("Configured serial {0} unavailable; falling back to connected device: {1}" -f $env:ANDROID_SERIAL, $serial)
        }
    }
}

if (-not $serial) {
    $serial = Get-FirstConnectedDeviceSerial -Adb $AdbPath
    if (-not $serial) {
        throw 'No Android device in state device.'
    }
}

if (-not (Wait-ForDeviceState -Adb $AdbPath -Serial $serial -TimeoutSeconds $DeviceWaitSeconds)) {
    throw ("Android device '{0}' is not in state 'device'. Check USB mode/authorization." -f $serial)
}

Write-Host ("Using Android device serial: {0}" -f $serial)

$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
& $AdbPath -s $serial forward --remove tcp:$LldbPort 2>$null
$removeForwardExitCode = $LASTEXITCODE
$ErrorActionPreference = $previousErrorActionPreference
if ($removeForwardExitCode -ne 0) {
    Write-Host ("No existing adb forward to remove on tcp:{0}; continuing." -f $LldbPort)
}

$packageName = $PackageActivity.Split('/')[0]

function Cleanup-StaleDebugProcesses {
    param(
        [string]$Adb,
        [string]$DeviceSerial,
        [string]$Pkg
    )

    # Best-effort cleanup: stale lldb-server can keep app process traced/stopped.
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'

    & $Adb -s $DeviceSerial shell "run-as $Pkg sh -c 'pidof lldb-server | xargs -r kill -9'" 2>$null | Out-Null
    & $Adb -s $DeviceSerial shell "pidof lldb-server | xargs -r kill -9" 2>$null | Out-Null

    $appPidOutput = & $Adb -s $DeviceSerial shell "pidof $Pkg" 2>$null
    if ($appPidOutput) {
        $appPids = $appPidOutput.Trim() -split '\s+'
        foreach ($appPid in $appPids) {
            if ($appPid) {
                & $Adb -s $DeviceSerial shell "run-as $Pkg sh -c 'kill -9 $appPid'" 2>$null | Out-Null
                & $Adb -s $DeviceSerial shell "kill -9 $appPid" 2>$null | Out-Null
            }
        }
    }

    $ErrorActionPreference = $previousErrorActionPreference
}

function Kill-AppProcess {
    param(
        [string]$Adb,
        [string]$DeviceSerial,
        [string]$Pkg,
        [string]$AppPid
    )

    if (-not $AppPid) {
        return
    }

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'

    & $Adb -s $DeviceSerial shell "run-as $Pkg sh -c 'kill -9 $AppPid'" 2>$null | Out-Null
    if ($LASTEXITCODE -ne 0) {
        & $Adb -s $DeviceSerial shell "kill -9 $AppPid" 2>$null | Out-Null
    }

    $ErrorActionPreference = $previousErrorActionPreference
}

if (-not $AttachOnly) {
    Cleanup-StaleDebugProcesses -Adb $AdbPath -DeviceSerial $serial -Pkg $packageName

    Write-Host ("Force-stopping existing app instance for package: {0}" -f $packageName)
    & $AdbPath -s $serial shell am force-stop $packageName
    if ($LASTEXITCODE -ne 0) {
        Write-Host ("adb force-stop for {0} returned non-zero; continuing." -f $packageName)
    }
    Start-Sleep -Milliseconds 500

    $startArgs = "start -n $PackageActivity"
    if ($WaitForDebugger) {
        $startArgs = "start -D -n $PackageActivity"
        Write-Host "Starting app in wait-for-debugger mode (-D)."
    }

    & $AdbPath -s $serial shell am $startArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host 'adb am start returned non-zero; continuing to debugger attach.'
    }
} else {
    Write-Host ("Attach-only mode: leaving running app untouched for package: {0}" -f $packageName)
}

function Get-AppPid {
    param(
        [string]$Adb,
        [string]$DeviceSerial,
        [string]$Pkg
    )

    $pidOutput = & $Adb -s $DeviceSerial shell "pidof $Pkg" 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $pidOutput) {
        return $null
    }

    $trimmed = $pidOutput.Trim()
    if (-not $trimmed) {
        return $null
    }

    # pidof can return multiple PIDs; use the first one.
    return ($trimmed -split '\s+')[0]
}

if ($StopAfterLaunch -and -not $AttachOnly) {
    $stoppedEarly = $false
    $maxAttempts = [Math]::Max(1, $DebugSocketWaitSeconds * 20)
    for ($attempt = 1; $attempt -le $maxAttempts; $attempt++) {
        $earlyPid = Get-AppPid -Adb $AdbPath -DeviceSerial $serial -Pkg $packageName
        if (-not $earlyPid) {
            Start-Sleep -Milliseconds 50
            continue
        }

        $previousErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'

        # Prefer run-as so signal is sent as app UID (shell user may not be allowed).
        & $AdbPath -s $serial shell "run-as $packageName sh -c 'kill -STOP $earlyPid'" 2>$null | Out-Null
        $killExitCode = $LASTEXITCODE

        if ($killExitCode -ne 0) {
            & $AdbPath -s $serial shell "kill -STOP $earlyPid" 2>$null | Out-Null
            $killExitCode = $LASTEXITCODE
        }

        $ErrorActionPreference = $previousErrorActionPreference

        if ($killExitCode -eq 0) {
            Write-Host ("Sent SIGSTOP to app pid: {0} before LLDB attach" -f $earlyPid)
            $stoppedEarly = $true
            break
        } else {
            # Keep trying briefly because PID can change during startup.
            if (($attempt % 20) -eq 0) {
                Write-Host ("Still trying to SIGSTOP app pid {0} before attach..." -f $earlyPid)
            }
            Start-Sleep -Milliseconds 50
        }
    }

    if (-not $stoppedEarly) {
        Write-Host "Could not confirm early app stop before attach; continuing."
    }
}

function Test-LldbServerRunning {
    param(
        [string]$Adb,
        [string]$DeviceSerial,
        [string]$Pkg
    )

    $pidOutput = & $Adb -s $DeviceSerial shell "run-as $Pkg sh -c 'pidof lldb-server'" 2>$null
    if ($LASTEXITCODE -eq 0 -and $pidOutput) {
        $trimmed = $pidOutput.Trim()
        if ($trimmed -match '^\d+(\s+\d+)*$') {
            return $true
        }
    }

    # Fallback for devices where pidof is unavailable in run-as shell.
    $psOutput = & $Adb -s $DeviceSerial shell "run-as $Pkg sh -c 'ps -A | grep lldb-server'" 2>$null
    if ($LASTEXITCODE -eq 0 -and $psOutput -and ($psOutput -match 'lldb-server')) {
        return $true
    }

    # Some devices block run-as process listing, but lldb-server may still be running.
    $globalPsOutput = & $Adb -s $DeviceSerial shell "ps -A | grep lldb-server" 2>$null
    if ($LASTEXITCODE -eq 0 -and $globalPsOutput -and ($globalPsOutput -match 'lldb-server')) {
        return $true
    }

    return $false
}

function Ensure-LldbServerInAppSandbox {
    param(
        [string]$Adb,
        [string]$DeviceSerial,
        [string]$Pkg
    )

    $hostLldbServer = Resolve-LldbServerHostPath
    if (-not $hostLldbServer) {
        throw "Could not locate host lldb-server binary in expected Android NDK paths."
    }

    $sandboxName = ('nlc-lldb-server-{0}' -f ([System.Guid]::NewGuid().ToString('N')))
    $sandboxPath = "./files/$sandboxName"

    $tmpPath = "/data/local/tmp/$sandboxName"
    & $Adb -s $DeviceSerial push $hostLldbServer $tmpPath | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to push lldb-server to device temp path."
    }

    & $Adb -s $DeviceSerial shell "chmod 755 $tmpPath" | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to chmod pushed lldb-server in /data/local/tmp."
    }

    & $Adb -s $DeviceSerial shell "run-as $Pkg sh -c 'mkdir -p files && cp $tmpPath $sandboxPath && chmod 700 $sandboxPath'" | Out-Null
    if ($LASTEXITCODE -eq 0) {
        return $sandboxPath
    }

    throw "run-as copy to app sandbox failed for lldb-server executable path: $sandboxPath"
}

$lldbServerRunPath = Ensure-LldbServerInAppSandbox -Adb $AdbPath -DeviceSerial $serial -Pkg $packageName

$forwarded = $false
$lastPidTried = $null
$moduleLoadAddress = $null
for ($attempt = 1; $attempt -le $DebugSocketWaitSeconds; $attempt++) {
    $appPid = Get-AppPid -Adb $AdbPath -DeviceSerial $serial -Pkg $packageName

    if (-not $appPid) {
        Start-Sleep -Seconds 1
        continue
    }

    $lastPidTried = $appPid

    if (-not $moduleLoadAddress) {
        $moduleLoadAddress = Get-ModuleLoadAddress -Adb $AdbPath -DeviceSerial $serial -Pkg $packageName -AppPid $appPid -ModuleFileName 'libnolimitconnect_arm64-v8a.so'
        if (-not $moduleLoadAddress) {
            Start-Sleep -Milliseconds 250
            continue
        }
    }

    # Start lldb-server inside app context and detach it so it survives the adb shell session.
    $attachServerOutput = & $AdbPath -s $serial shell "run-as $packageName sh -c 'nohup $lldbServerRunPath gdbserver --attach $appPid localhost:$LldbPort >/dev/null 2>&1 </dev/null &'" 2>&1
    if ($LASTEXITCODE -ne 0) {
        if ($attachServerOutput) {
            Write-Host ($attachServerOutput | Out-String)
        }
        Start-Sleep -Seconds 1
        continue
    }

    & $AdbPath -s $serial forward tcp:$LldbPort tcp:$LldbPort | Out-Null
    $forwardExitCode = $LASTEXITCODE
    if ($forwardExitCode -eq 0) {
        # IMPORTANT: do not open a probe socket to tcp:$LldbPort here.
        # A probe connection can consume/terminate a one-shot gdbserver session
        # before CodeLLDB performs the real handshake.
        if (Test-LldbServerRunning -Adb $AdbPath -DeviceSerial $serial -Pkg $packageName) {
            $forwarded = $true
            break
        }

        # Give lldb-server a short grace period to appear in process listings.
        Start-Sleep -Milliseconds 500
        if (Test-LldbServerRunning -Adb $AdbPath -DeviceSerial $serial -Pkg $packageName) {
            $forwarded = $true
            break
        }

        # Some devices block process listing under run-as; trust forward in attach-only mode.
        if ($AttachOnly) {
            Write-Host "Attach-only mode: lldb-server visibility inconclusive; proceeding with forwarded socket."
            $forwarded = $true
            break
        }
    }

    & $AdbPath -s $serial forward --remove tcp:$LldbPort 2>$null | Out-Null
    Start-Sleep -Seconds 1
}

if (-not $forwarded) {
    if ($StopAfterLaunch -and -not $AttachOnly -and $lastPidTried) {
        Write-Host ("Attach failed; killing app pid {0} so it does not remain running between launches." -f $lastPidTried)
        Kill-AppProcess -Adb $AdbPath -DeviceSerial $serial -Pkg $packageName -AppPid $lastPidTried
    }
    throw "Failed to start/attach lldb-server for package $packageName (last pid tried: $lastPidTried)"
}

$modulePreferredBase = $null
if ($resolvedWorkspace) {
    $modulePreferredBase = Get-ModulePreferredBase -Workspace $resolvedWorkspace -BuildSubdir 'build/android-arm64-debug/nolimitgui' -ModuleFileName 'libnolimitconnect_arm64-v8a.so'
}
if (-not $moduleLoadAddress) {
    Write-Host 'Could not determine runtime load address for libnolimitconnect_arm64-v8a.so.'
}

if (-not $modulePreferredBase) {
    Write-Host 'Could not determine preferred load address for libnolimitconnect_arm64-v8a.so.'
}

if ($resolvedWorkspace) {
    Write-LldbModuleLoadCommands -Workspace $resolvedWorkspace -BuildSubdir 'build/android-arm64-debug/nolimitgui' -ModuleFileName 'libnolimitconnect_arm64-v8a.so' -ModuleLoadAddress $moduleLoadAddress -ModulePreferredBase $modulePreferredBase
}

Write-Host ("Forwarded tcp:{0} to device tcp:{0} (attached pid: {1})" -f $LldbPort, $lastPidTried)
exit 0
