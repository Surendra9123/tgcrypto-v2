#  Pyrogram - Telegram MTProto API Client Library for Python
#  Copyright (C) 2017-present Dan <https://github.com/delivrance>
#
#  This file is part of Pyrogram.
#
#  Pyrogram is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.

import unittest

import tgcrypto

try:
    # importlib.metadata is stdlib on Python >= 3.8; older versions skip the
    # metadata cross-check (the attribute presence test below still runs).
    import importlib.metadata as _metadata
except ImportError:  # pragma: no cover - Python 3.7
    _metadata = None


class TestVersion(unittest.TestCase):
    def test_has_version_string(self):
        self.assertTrue(hasattr(tgcrypto, "__version__"))
        self.assertIsInstance(tgcrypto.__version__, str)
        self.assertTrue(tgcrypto.__version__)

    def test_matches_package_metadata(self):
        if _metadata is None:
            self.skipTest("importlib.metadata unavailable (Python < 3.8)")
        # The compiled-in __version__ must match the installed metadata
        # (both derive from tgcrypto/version.h).
        try:
            meta = _metadata.version("tgcrypto")
        except _metadata.PackageNotFoundError:
            self.skipTest("tgcrypto distribution metadata not installed")
        self.assertEqual(tgcrypto.__version__, meta)
