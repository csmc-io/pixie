/*
 * Copyright © 2024- Cosmic Observe, Inc.
 * All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of Cosmic Observe, Inc. and its suppliers,
 * if any.
 *
 * SPDX-License-Identifier: Proprietary
 */

import * as React from 'react';

export const termsUri = 'https://docs.csmc.ai/terms-and-conditions/tos/';
export const privacyUri = 'https://docs.csmc.ai/terms-and-conditions/privacy-policy/';

export const TermsAndPrivacy = React.memo<{ classes: Record<'text', string> }>(({ classes }) => {
  return (<>
    <a href={termsUri} className={classes.text}>
      Terms & Conditions
    </a>
    <a href={privacyUri} className={classes.text}>
      Privacy Policy
    </a>
  </>);
});
TermsAndPrivacy.displayName = 'TermsAndPrivacy';
