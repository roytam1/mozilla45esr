# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import time

from marionette_driver import By, Wait
from marionette_driver.errors import MarionetteException

from firefox_puppeteer.testcases import FirefoxTestCase


class TestSecurityNotification(FirefoxTestCase):

    def setUp(self):
        FirefoxTestCase.setUp(self)

        self.urls = [
            # Invalid cert page
            'https://ssl-expired.mozqa.com',
            # Secure page
            'https://ssl-ev.mozqa.com/',
            # Insecure page
            'http://no-ssl.mozqa.com'
        ]

        self.identity_box = self.browser.navbar.locationbar.identity_box

    def test_invalid_cert(self):
        with self.marionette.using_context('content'):
            # Go to a site that has an invalid (expired) cert
            self.assertRaises(MarionetteException, self.marionette.navigate, self.urls[0])

            # Wait for the DOM to receive events
            time.sleep(1)

            # Verify the text in Technical Content contains the page with invalid cert
            text = self.marionette.find_element(By.ID, 'technicalContentText')
            self.assertIn(self.urls[0][8:], text.get_attribute('textContent'))

            # Verify the "Go Back" and "Advanced" buttons appear
            self.assertIsNotNone(self.marionette.find_element(By.ID, 'returnButton'))
            self.assertIsNotNone(self.marionette.find_element(By.ID, 'advancedButton'))

            # Verify the error code is correct
            self.assertIn('SEC_ERROR_EXPIRED_CERTIFICATE', text.get_attribute('textContent'))

    def test_secure_website(self):
        with self.marionette.using_context('content'):
            self.marionette.navigate(self.urls[1])

        Wait(self.marionette).until(lambda _: (
            self.identity_box.get_attribute('className') == 'verifiedIdentity')
        )

    def test_insecure_website(self):
        with self.marionette.using_context('content'):
            self.marionette.navigate(self.urls[2])

        Wait(self.marionette).until(lambda _: (
            self.identity_box.get_attribute('className') == 'unknownIdentity')
        )
