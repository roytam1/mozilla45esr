/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * The origin of this IDL file is
 * http://dev.w3.org/csswg/cssom/
 */

interface CSSRule;

interface CSSStyleSheet : StyleSheet {
  [Pure]
  readonly attribute CSSRule? ownerRule;
  [Throws]
  readonly attribute CSSRuleList cssRules;
  [Throws]
  unsigned long insertRule(DOMString rule, optional unsigned long index = 0);
  [Throws]
  void deleteRule(unsigned long index);

  // Non-standard WebKit things, see https://github.com/w3c/csswg-drafts/pull/3900.
  [Throws, BinaryName="cssRules"]
  readonly attribute CSSRuleList rules;
  [Throws, BinaryName="deleteRule"]
  void removeRule(optional unsigned long index = 0);
  [Throws]
  long addRule(optional DOMString selector = "undefined", optional DOMString style = "undefined", optional unsigned long index);
};
