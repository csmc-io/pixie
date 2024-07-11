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

import * as logo from 'assets/images/cosmic-logo-dark-bg-monochrome.svg';

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

  return (
    <div className={classes.logoContainer}>
      <img
        className={classes.logo}
        src={logo}
        style={{
          color,
          fill: color,
        }}
      />
    </div>
  );
});
Logo.displayName = 'Logo';
