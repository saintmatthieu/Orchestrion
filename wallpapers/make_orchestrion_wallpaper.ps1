Add-Type -AssemblyName System.Drawing

# Sized to comfortably exceed common monitors. Viewport-anchored (per submodule patch),
# so any repeat is hidden by the symmetric top/bottom mahogany.
$W = 2560
$H = 1440

$bmp = New-Object System.Drawing.Bitmap $W, $H
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

# --- Palette ---
$mahogany = [System.Drawing.Color]::FromArgb(61, 31, 26)    # #3D1F1A — top & bottom
$cream    = [System.Drawing.Color]::FromArgb(240, 229, 200) # #F0E5C8 — center band
$gold     = [System.Drawing.Color]::FromArgb(212, 168, 88)  # #D4A858 — accent
$goldSoft = [System.Drawing.Color]::FromArgb(160, 212, 168, 88)

# --- Vertical gradient: mahogany -> cream -> mahogany, with a soft-edged cream plateau ---
# Strategy: paint per-row colors blending between the two endpoints based on a smoothstep
# centered on H/2 with a configurable plateau width.
$plateauHalf = [int]($H * 0.18)   # half-height of fully-cream center band
$falloff     = [int]($H * 0.22)   # additional pixels for soft transition each side

function Lerp($a, $b, $t) { return $a + ($b - $a) * $t }
function Smoothstep($t) {
    if ($t -le 0) { return 0 }
    if ($t -ge 1) { return 1 }
    return $t * $t * (3 - 2 * $t)
}
function MixColor($cA, $cB, $t) {
    $r = [int][Math]::Round((Lerp $cA.R $cB.R $t))
    $g = [int][Math]::Round((Lerp $cA.G $cB.G $t))
    $b = [int][Math]::Round((Lerp $cA.B $cB.B $t))
    return [System.Drawing.Color]::FromArgb($r, $g, $b)
}

$cy = $H / 2
for ($y = 0; $y -lt $H; $y++) {
    $dist = [Math]::Abs($y - $cy)
    if ($dist -le $plateauHalf) {
        $t = 0.0
    } elseif ($dist -ge ($plateauHalf + $falloff)) {
        $t = 1.0
    } else {
        $t = Smoothstep (($dist - $plateauHalf) / $falloff)
    }
    $rowColor = MixColor $cream $mahogany $t
    $rowPen   = New-Object System.Drawing.Pen $rowColor, 1
    $g.DrawLine($rowPen, 0, $y, $W, $y)
    $rowPen.Dispose()
}

# --- Gold ornament near the bottom: thin centered rule with a small diamond accent ---
# Positioned ~85% down, well inside the lower mahogany band.
$ornY    = [int]($H * 0.85)
$ornW    = [int]($W * 0.32)
$ornX1   = [int](($W - $ornW) / 2)
$ornX2   = $ornX1 + $ornW
$ornXMid = [int](($ornX1 + $ornX2) / 2)
$gap     = 18

# Two short rules with a centered diamond between them
$rulePen = New-Object System.Drawing.Pen $gold, 1.4
$g.DrawLine($rulePen, $ornX1, $ornY, ($ornXMid - $gap), $ornY)
$g.DrawLine($rulePen, ($ornXMid + $gap), $ornY, $ornX2, $ornY)
$rulePen.Dispose()

# Tiny diamond in the middle (rotated square, 8 px half-diagonal)
$dHalf  = 6
$diamondPts = @(
    (New-Object System.Drawing.Point $ornXMid, ($ornY - $dHalf)),
    (New-Object System.Drawing.Point ($ornXMid + $dHalf), $ornY),
    (New-Object System.Drawing.Point $ornXMid, ($ornY + $dHalf)),
    (New-Object System.Drawing.Point ($ornXMid - $dHalf), $ornY)
)
$goldBrush = New-Object System.Drawing.SolidBrush $gold
$g.FillPolygon($goldBrush, $diamondPts)
$goldBrush.Dispose()

# Soft outer halo on the rule for a subtle glow (low alpha, slightly thicker, drawn under by re-stroking after — order doesn't matter at this alpha)
$haloPen = New-Object System.Drawing.Pen $goldSoft, 3.5
$g.DrawLine($haloPen, $ornX1, $ornY, ($ornXMid - $gap), $ornY)
$g.DrawLine($haloPen, ($ornXMid + $gap), $ornY, $ornX2, $ornY)
$haloPen.Dispose()

# --- Save as JPG ---
$out = Join-Path $PSScriptRoot "orchestrion_parchment.jpg"

$encoders = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders()
$jpegEnc  = $encoders | Where-Object { $_.FormatID -eq [System.Drawing.Imaging.ImageFormat]::Jpeg.Guid }
$params   = New-Object System.Drawing.Imaging.EncoderParameters 1
$params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter ([System.Drawing.Imaging.Encoder]::Quality), 90L

$bmp.Save($out, $jpegEnc, $params)
$g.Dispose(); $bmp.Dispose()

Write-Output "Wrote $out  ($W x $H)"
