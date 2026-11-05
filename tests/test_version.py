#  Pyrogram - Telegram MTProto API Client Library for Python
#  Copyright (C) 2017-present Dan <https://github.com/delivrance>
#
#  This file is part of Pyrogram.
#
#  Pyrogram is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.

import importlib.metadata
import unittest

import tgcrypto


class TestVersion(unittest.TestCase):
    def test_has_version_string(self):
        self.assertTrue(hasattr(tgcrypto, "__version__"))
        self.assertIsInstance(tgcrypto.__version__, str)
        self.assertTrue(tgcrypto.__version__)

    def test_matches_package_metadata(self):
        # The compiled-in __version__ must match the installed metadata
        # (both derive from tgcrypto/version.h).
        try:
            meta = importlib.metadata.version("tgcrypto")
        except importlib.metadata.PackageNotFoundError:
            self.skipTest("tgcrypto distribution metadata not installed")
        self.assertEqual(tgcrypto.__version__, meta)
