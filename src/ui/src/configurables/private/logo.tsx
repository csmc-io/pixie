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

import { useTheme } from '@mui/material/styles';
import { createStyles, makeStyles } from '@mui/styles';

import * as darkLogo from 'assets/images/cosmic-logo-dark-bg-monochrome.svg';
import * as lightLogo from 'assets/images/cosmic-logo-light-bg-monochrome.svg';

const useStyles = makeStyles(() => createStyles({
  logo: {
    height: '100%',
    width: 'auto',
  },
  logoContainer: {
    height: '100% !important',
    display: 'flex',
  },
}), { name: 'Logo' });

export const Logo = React.memo<{ color?: string }>(({ color }) => {
  const classes = useStyles();
  const theme = useTheme();
  const { mode } = theme.palette;

  return (
    <div className={classes.logoContainer}>
      <img
        className={classes.logo}
        src={mode == 'light' ? lightLogo : darkLogo}
        style={{
          color,
          fill: color,
        }}
      />
    </div>
  );
});
Logo.displayName = 'Logo';
