/*
 * Copyright © 2024–2026 Cosmic Observe, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

import * as React from 'react';

import { Button } from '@mui/material';
import { useTheme } from '@mui/material/styles';
import { CookieConsent } from 'react-cookie-consent';

/* eslint-disable react-memo/require-usememo */
export function PixieCookieBanner(): React.ReactElement {
  const theme = useTheme();
  return (
    <CookieConsent
      location='bottom'
      buttonText='Accept'
      cookieName='cc_cookie'
      expires={365}
      ButtonComponent={Button}
      disableStyles={true}
      style={{
        background: theme.palette.background.one,
        opacity: '0.9',
        alignItems: 'baseline',
        display: 'flex',
        flexWrap: 'wrap',
        justifyContent: 'center',
        left: '0',
        position: 'fixed',
        width: '100%',
        zIndex: 1400,
      }}
      buttonStyle={{
        border: 'solid 1px',
        color: theme.palette.foreground.two,
        backgroundColor: theme.palette.background.three,
        boxShadow: 'none',
        cursor: 'pointer',
        flex: '0 0 auto',
        padding: '5px 10px',
        margin: '15px',
      }}
      contentStyle={{
        flex: '0 auto',
        margin: '15px',
      }}
      overlayStyle={{
        position: 'fixed',
        left: 0,
        top: 0,
        width: '100%',
        height: '100%',
        zIndex: '1400',
      }}
    >
    This site uses cookies to improve your experience. By using this service, you opt into their usage as
    outlined in our <a href='https://docs.csmc.ai/terms-and-conditions/cookie-policy'>Cookie Policy</a>.
    </CookieConsent>
  );
}
PixieCookieBanner.displayName = 'PixieCookieBanner';
