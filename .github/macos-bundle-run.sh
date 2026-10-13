#!/bin/bash
# Runs the 14 rendering tests that fail on the hosted macOS runner, then all GPU suites.
# Results are written beside this script.
cd "$(dirname "$0")"
xattr -dr com.apple.quarantine . 2>/dev/null
export DYLD_LIBRARY_PATH="$PWD/libs"
export SDL_VULKAN_LIBRARY="$PWD/libs/libvulkan.1.dylib"
export VK_DRIVER_FILES="$PWD/MoltenVK_icd.json"
./vulkaninfo --summary > vulkaninfo.txt 2>&1
F='MgDrawPrimRender.LinearAtlasCellsKeepTopOutlinesWithoutSamplingTheNextCell:MgShadow.OverlappingCastersShadeTheirUnionNotTheirSum:MgVisual.DrawManagerPathMatchesDirect:MgVisual.DirectMaterialUsesItsOwnTextureWrap:MgVisual.QueuedMaterialRetainsItsOwnTextureWrap:MglibAspect.MenuFormsHideParkedStatsWithoutClippingEnteringPanel:MglibPresent.BackBufferCapturePreservesWidePixelsAndHoldsUntilRecaptured:MglibSeam.ReusedTextureKeyDoesNotSampleRetiredRenderTarget:MenuIconGpu.ItemDrawSamplesSelectedAtlasCellsInCanonicalAndDisplayFrames:MenuIconGpu.EffectUsesSelectedBakedRgbaAtlasAndRestoresNormally:MgTextureGpu.HighResolutionKeepsLogicalCpuStrideAndSpriteBounds:GfxGpuMeshDc2.RefSphere:GfxGpuMeshDc2.Prog0CullModeFollowsFlagAndMirror:GfxGpuDisplayList.PresentationCompositeDoesNotWriteTransparentDepth'
./darkchronicle_tests --gtest_filter="$F" > result-14.txt 2>&1
echo "14 tests that fail on the CI runner: exit $?" | tee summary.txt
./darkchronicle_tests --gtest_filter='Mg*:Mglib*:MenuIconGpu*:MgTextureGpu*:GfxGpu*:*/GfxGpu*' > result-gpu.txt 2>&1
echo "all GPU suites: exit $?" | tee -a summary.txt
grep -E '^\[  (PASSED|FAILED) ' result-14.txt result-gpu.txt | tail -40 | tee -a summary.txt
echo "Done. Send back summary.txt, result-14.txt, result-gpu.txt and vulkaninfo.txt."
