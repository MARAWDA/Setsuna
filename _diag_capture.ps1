$s = New-Object System.IO.Ports.SerialPort COM12,115200,None,8,One
$s.DtrEnable = $false
$s.RtsEnable = $false
$s.ReadTimeout = 500
$s.Open()
$s.RtsEnable = $true
Start-Sleep -Milliseconds 100
$s.RtsEnable = $false
$sb = New-Object System.Text.StringBuilder
$deadline = (Get-Date).AddSeconds(15)
while ((Get-Date) -lt $deadline) {
  try { $sb.Append($s.ReadExisting()) | Out-Null } catch {}
  Start-Sleep -Milliseconds 200
}
$s.Close()
$sb.ToString() | Set-Content "C:\Users\wisht\New folder\Setsuna\_diag_out.txt" -Encoding utf8
