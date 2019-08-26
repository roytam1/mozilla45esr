/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsASCIIMask.h"

namespace mozilla {

MOZ_CONSTEXPR bool TestWhitespace(char c)
{
  return c == '\f' || c == '\t' || c == '\r' || c == '\n' || c == ' ';
}
MOZ_CONSTEXPR_VAR ASCIIMaskArray sWhitespaceMask = CreateASCIIMask(TestWhitespace);

MOZ_CONSTEXPR bool TestCRLF(char c)
{
  return c == '\r' || c == '\n';
}
MOZ_CONSTEXPR_VAR ASCIIMaskArray sCRLFMask = CreateASCIIMask(TestCRLF);

MOZ_CONSTEXPR bool TestCRLFTab(char c)
{
  return c == '\r' || c == '\n' || c == '\t';
}
MOZ_CONSTEXPR_VAR ASCIIMaskArray sCRLFTabMask = CreateASCIIMask(TestCRLFTab);

MOZ_CONSTEXPR bool TestZeroToNine(char c)
{
  return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
         c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
}
MOZ_CONSTEXPR_VAR ASCIIMaskArray sZeroToNineMask = CreateASCIIMask(TestZeroToNine);

const ASCIIMaskArray& ASCIIMask::MaskWhitespace()
{
  return sWhitespaceMask;
}

const ASCIIMaskArray& ASCIIMask::MaskCRLF()
{
  return sCRLFMask;
}

const ASCIIMaskArray& ASCIIMask::MaskCRLFTab()
{
  return sCRLFTabMask;
}

const ASCIIMaskArray& ASCIIMask::Mask0to9()
{
  return sZeroToNineMask;
}

} // namespace mozilla
