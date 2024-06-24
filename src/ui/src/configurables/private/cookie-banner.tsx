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
    outlined in our <a href='https://csmc.ai/docs/terms-and-conditions/cookie-policy'>Cookie Policy</a>.
    </CookieConsent>
  );
}
PixieCookieBanner.displayName = 'PixieCookieBanner';
