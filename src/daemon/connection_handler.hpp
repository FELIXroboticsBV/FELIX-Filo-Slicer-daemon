// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#pragma once

#include <functional>

namespace felix_daemon {
enum class HandlerResult { KeepRuning, Shutdown };

using connection_handler = std::function<HandlerResult(int client_fd)>;
} // namespace felix_daemon