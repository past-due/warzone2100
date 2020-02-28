Function Req {
   Param(
       [Parameter(Mandatory=$True)]
       [hashtable]$Params,
       [int]$Retries = 1,
       [int]$SecondsDelay = 2
   )

   $method = $Params['Method']
   $url = $Params['Uri']

   $cmd = { Write-Host "$method $url..." -NoNewline; Invoke-WebRequest @Params }

   $retryCount = 0
   $completed = $false
   $response = $null

   while (-not $completed) {
       try {
           $response = Invoke-Command $cmd -ArgumentList $Params
           # if ($response.StatusCode -ne 200) {
           #     throw "Expecting reponse code 200, was: $($response.StatusCode)"
           # }
           $completed = $true
       } catch {
           Write-Host $_.Exception.GetType().FullName
           Write-Host $_.Exception.Message
           if ($retrycount -ge $Retries) {
               Write-Warning "Request to $url failed the maximum number of $retryCount times."
               throw
           } else {
               Write-Warning "Request to $url failed. Retrying in $SecondsDelay seconds."
               Start-Sleep $SecondsDelay
               $retrycount++
           }
       }
   }

   Write-Host "OK ($($response.StatusCode))"
   return $response
}
