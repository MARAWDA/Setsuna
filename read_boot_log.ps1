$port = "COM12"
$serial = New-Object System.IO.Ports.SerialPort $port,115200,None,8,One
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$serial.ReadTimeout = 500
$serial.Open()
Start-Sleep -Milliseconds 100
$serial.DtrEnable = $true
Start-Sleep -Milliseconds 150
$serial.DtrEnable = $false
$all = ""
for ($i = 0; $i -lt 30; $i++) {
    $c = $serial.ReadExisting()
    if ($c) { $all += $c }
    Start-Sleep -Milliseconds 400
}
$serial.Close()
Set-Content -Path "C:\Users\wisht\New folder\Setsuna\shim_boot_log4.txt" -Value $all -Encoding UTF8
