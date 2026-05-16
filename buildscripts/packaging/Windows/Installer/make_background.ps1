Add-Type -AssemblyName System.Drawing

$W = 986
$H = 624
$bandW = 324  # match MSS layout proportion so installer text on the right stays readable

$bmp = New-Object System.Drawing.Bitmap $W, $H
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic

# --- Right area: warm cream / aged parchment ---
$cream      = [System.Drawing.Color]::FromArgb(244, 238, 224)
$creamEdge  = [System.Drawing.Color]::FromArgb(236, 228, 210)
$rightRect  = New-Object System.Drawing.Rectangle 0, 0, $W, $H
$rightBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush $rightRect, $cream, $creamEdge, ([System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
$g.FillRectangle($rightBrush, $rightRect)
$rightBrush.Dispose()

# --- Left band: deep mahogany with subtle vertical gradient (top slightly lighter) ---
$mahoTop = [System.Drawing.Color]::FromArgb(74, 36, 30)   # #4A241E
$mahoBot = [System.Drawing.Color]::FromArgb(42, 20, 17)   # #2A1411
$bandRect  = New-Object System.Drawing.Rectangle 0, 0, $bandW, $H
$bandBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush $bandRect, $mahoTop, $mahoBot, ([System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
$g.FillRectangle($bandBrush, $bandRect)
$bandBrush.Dispose()

# --- Soft transition strip from band to cream (avoids hard MSS-style cut) ---
$strip      = New-Object System.Drawing.Rectangle ($bandW), 0, 28, $H
$stripFrom  = [System.Drawing.Color]::FromArgb(255, 42, 20, 17)
$stripTo    = [System.Drawing.Color]::FromArgb(0,   244, 238, 224)
$stripBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush $strip, $stripFrom, $stripTo, ([System.Drawing.Drawing2D.LinearGradientMode]::Horizontal)
$g.FillRectangle($stripBrush, $strip)
$stripBrush.Dispose()

# --- Thin gold rule between band and cream ---
$goldLine = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(180, 201, 150, 60)), 1.2
$g.DrawLine($goldLine, $bandW, 0, $bandW, $H)
$goldLine.Dispose()

# --- "Orchestrion" wordmark, italic serif, soft gold ---
$gold       = [System.Drawing.Color]::FromArgb(212, 168, 88)    # #D4A858
$goldBrush  = New-Object System.Drawing.SolidBrush $gold
$titleFont  = New-Object System.Drawing.Font "Palatino Linotype", 36, ([System.Drawing.FontStyle]::Italic), ([System.Drawing.GraphicsUnit]::Pixel)
$title      = "Orchestrion"
$titleSize  = $g.MeasureString($title, $titleFont)
$titleX     = ($bandW - $titleSize.Width) / 2
$titleY     = ($H - $titleSize.Height) / 2 - 6
$g.DrawString($title, $titleFont, $goldBrush, $titleX, $titleY)

# --- Tagline beneath wordmark, smaller, slightly dimmer ---
$tagFont    = New-Object System.Drawing.Font "Palatino Linotype", 13, ([System.Drawing.FontStyle]::Regular), ([System.Drawing.GraphicsUnit]::Pixel)
$tagBrush   = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(190, 180, 150, 95))
$tag        = "play the score"
$tagSize    = $g.MeasureString($tag, $tagFont)
$tagX       = ($bandW - $tagSize.Width) / 2
$tagY       = $titleY + $titleSize.Height + 2
$g.DrawString($tag, $tagFont, $tagBrush, $tagX, $tagY)

# --- Hairline ornament under tagline ---
$ornPen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(120, 201, 150, 60)), 1
$ornY   = [int]($tagY + $tagSize.Height + 12)
$ornW   = 70
$ornX1  = [int]($bandW / 2 - $ornW / 2)
$ornX2  = $ornX1 + $ornW
$g.DrawLine($ornPen, $ornX1, $ornY, $ornX2, $ornY)
$ornPen.Dispose()

$titleFont.Dispose(); $tagFont.Dispose(); $goldBrush.Dispose(); $tagBrush.Dispose()

# --- Save ---
$out = Join-Path $PSScriptRoot "installer_background_wix.png"
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()

Write-Output "Wrote $out"

# ============================================================
#                    BANNER (top of dialogs)
# ============================================================
$BW = 986
$BH = 116

$bnr  = New-Object System.Drawing.Bitmap $BW, $BH
$gb   = [System.Drawing.Graphics]::FromImage($bnr)
$gb.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$gb.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit
$gb.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic

# Cream background, matching the dialog right area for visual continuity
$bnrRect  = New-Object System.Drawing.Rectangle 0, 0, $BW, $BH
$bnrTop   = [System.Drawing.Color]::FromArgb(244, 238, 224)
$bnrBot   = [System.Drawing.Color]::FromArgb(236, 228, 210)
$bnrBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush $bnrRect, $bnrTop, $bnrBot, ([System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
$gb.FillRectangle($bnrBrush, $bnrRect)
$bnrBrush.Dispose()

# Thin gold hairline along the bottom edge
$hairPen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(140, 201, 150, 60)), 1
$gb.DrawLine($hairPen, 0, ($BH - 1), $BW, ($BH - 1))
$hairPen.Dispose()

# Emblem on the right: mahogany disc with gold italic "O"
$emblemD = 84
$emblemX = $BW - $emblemD - 24
$emblemY = ($BH - $emblemD) / 2

$mahoEm = New-Object System.Drawing.Drawing2D.LinearGradientBrush (
    (New-Object System.Drawing.Rectangle $emblemX, $emblemY, $emblemD, $emblemD),
    ([System.Drawing.Color]::FromArgb(74, 36, 30)),
    ([System.Drawing.Color]::FromArgb(42, 20, 17)),
    ([System.Drawing.Drawing2D.LinearGradientMode]::ForwardDiagonal)
)
$gb.FillEllipse($mahoEm, $emblemX, $emblemY, $emblemD, $emblemD)
$mahoEm.Dispose()

# Subtle gold ring
$ringPen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(200, 201, 150, 60)), 1.4
$gb.DrawEllipse($ringPen, $emblemX + 0.7, $emblemY + 0.7, $emblemD - 1.4, $emblemD - 1.4)
$ringPen.Dispose()

# Italic serif "O" centered in the disc
$gold        = [System.Drawing.Color]::FromArgb(212, 168, 88)
$emblemBrush = New-Object System.Drawing.SolidBrush $gold
$emblemFont  = New-Object System.Drawing.Font "Palatino Linotype", 52, ([System.Drawing.FontStyle]::Italic), ([System.Drawing.GraphicsUnit]::Pixel)
$letter      = "O"
$letterSize  = $gb.MeasureString($letter, $emblemFont)
$letterX     = $emblemX + ($emblemD - $letterSize.Width) / 2
$letterY     = $emblemY + ($emblemD - $letterSize.Height) / 2 - 2
$gb.DrawString($letter, $emblemFont, $emblemBrush, $letterX, $letterY)
$emblemFont.Dispose()
$emblemBrush.Dispose()

$bnrOut = Join-Path $PSScriptRoot "installer_banner_wix.png"
$bnr.Save($bnrOut, [System.Drawing.Imaging.ImageFormat]::Png)
$gb.Dispose(); $bnr.Dispose()

Write-Output "Wrote $bnrOut"
