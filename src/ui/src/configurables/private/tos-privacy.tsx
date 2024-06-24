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

export const TermsAndPrivacy = React.memo<{ classes: Record<'text', string> }>(({ classes }) => {
  return (<>
    <a href='https://csmc.ai/docs/terms-and-conditions/tos/' className={classes.text}>
      Terms & Conditions
    </a>
    <a href='https://csmc.ai/docs/terms-and-conditions/privacy-policy/' className={classes.text}>
      Privacy Policy
    </a>
  </>);
});
TermsAndPrivacy.displayName = 'TermsAndPrivacy';
