param([string]$Path)
$bytes = [System.IO.File]::ReadAllBytes($Path)
function RBE($o) { ($script:bytes[$o] -shl 24) -bor ($script:bytes[$o+1] -shl 16) -bor ($script:bytes[$o+2] -shl 8) -bor $script:bytes[$o+3] }
function RWS($o,$n) { $s=''; for($i=0;$i -lt $n;$i++){$c=($script:bytes[$o+$i*2] -shl 8) -bor $script:bytes[$o+$i*2+1]; if($c){$s+=[char]$c}}; $s }

$off=0
$ver=RBE $off; $off+=4
$pc=RBE $off; $off+=4
Write-Host "File size: $($bytes.Length) bytes"
Write-Host "Version: $ver  ParamTypes: $pc"

# DLC_FILE_PARAM struct: DWORD dwType(4) + DWORD dwWchCount(4) + WCHAR[1](2) = 10, padded to 12
# Total WCHARs = 2 + dwWchCount (struct has space for 2 built-in)
# Total size = 12 + dwWchCount * 2

$ptypes=@()
for($p=0;$p -lt $pc;$p++){
    $pt=RBE $off; $off+=4
    $wc=RBE $off; $off+=4
    $totalWch = 2 + $wc
    $nm=RWS $off $totalWch; $off += $totalWch * 2
    $ptypes+=$pt
    Write-Host "  Param[$p]: type=$pt wchCount=$wc name='$nm'"
}

$fc=RBE $off; $off+=4
Write-Host "FileCount: $fc"
Write-Host ""

# DLC_FILE_DETAILS: uint uiFileSize(4) + DWORD dwType(4) + DWORD dwWchCount(4) + WCHAR[1](2) = 14, padded to 16
# Total WCHARs = 2 + dwWchCount
# Total size = 16 + dwWchCount * 2

$fileHeaders = @()
$headersStart = $off
for($f=0;$f -lt $fc;$f++){
    $fsz = RBE $off; $off+=4
    $ftype = RBE $off; $off+=4
    $fwc = RBE $off; $off+=4
    $totalWch = 2 + $fwc
    $fname = RWS $off $totalWch; $off += $totalWch * 2
    $fileHeaders += @{ Size=$fsz; Type=$ftype; WchCount=$fwc; Name=$fname }
    Write-Host ("Header[{0}]: size={1} type={2} name='{3}'" -f $f,$fsz,$ftype,$fname)
}

Write-Host ""
Write-Host "Data section starts at offset: $off"
Write-Host ""

# Now per-file param blocks + data blobs
for($f=0;$f -lt $fc;$f++){
    $h = $fileHeaders[$f]
    $pCount = RBE $off; $off+=4
    $pstrs = @()
    for($pi=0;$pi -lt $pCount;$pi++){
        $pt2 = RBE $off; $off+=4
        $wc2 = RBE $off; $off+=4
        $tw2 = 2 + $wc2
        $sv = RWS $off $tw2; $off += $tw2 * 2
        $pstrs += $sv
    }
    $dataOff = $off
    $endOff = [Math]::Min($off+15,$bytes.Length-1)
    $preview = ($bytes[$off..$endOff] | ForEach-Object {'{0:X2}' -f $_}) -join ' '
    Write-Host ("File[{0}]: '{1}' size={2} params=[{3}]" -f $f,$h.Name,$h.Size,($pstrs -join ' | '))
    Write-Host "  DataOffset=$dataOff  First16: $preview"

    # For GRF files, check compression type
    if ($h.Name -match '\.grf' -or ($pstrs | Where-Object {$_ -match '\.grf'})) {
        if ($h.Size -ge 3) {
            $grfVer = ($bytes[$dataOff] -shl 8) -bor $bytes[$dataOff+1]
            $comprType = $bytes[$dataOff+2]
            Write-Host "  ** GRF: version=$grfVer compressionType=$comprType (0=None,1=RLE,2=LZX,3=ZLIB)"
        }
    }
    $off += $h.Size
}
Write-Host ""
Write-Host "Parsed to offset $off / $($bytes.Length)"
