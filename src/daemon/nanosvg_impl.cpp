// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

// orca GUI is implementing this but since we are not linking against it we do not have access to it
// if it was not here the daemon would not compile
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"