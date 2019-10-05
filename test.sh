#!/bin/bash

# This test script is expected to run in Multipass or Docker image
# under test environment as run by Travis CI for example - it depends
# on installing packages, adding users and sudo

set -exo pipefail

sudo apt-get install -y pamtester tacacs+ expect

sudo tee /etc/tacacs+/tac_plus.conf <<_EOT
accounting file = /var/log/tac_plus.acct

key = testkey123

user = testuser1 {
        global = cleartext "testpass123"
        service = ppp protocol = ip {
                addr=1.2.3.4
        }
}
user = testuser2 {
        global = cleartext "testpass123"
        service = ppp protocol = ip {
                addr=2.3.4.5
        }
}
_EOT

sudo service tacacs_plus restart

tail -20 /var/log/syslog

expect <<_EOT
set timeout -1
spawn pamtester -v -I rhost=localhost test testuser1 authenticate acct_mgmt open_session close_session
match_max 100000
expect -exact "pamtester: invoking pam_start(test, testuser1, ...)\r
pamtester: performing operation - authenticate\r
Password: "
send -- "testpass123\r"
expect "pamtester: successfully authenticated\r"
expect eof
_EOT

tail -20 /var/log/syslog

expect <<_EOT
set timeout -1
spawn pamtester -v -I rhost=localhost test testuserX authenticate acct_mgmt open_session close_session
match_max 100000
expect -exact "pamtester: invoking pam_start(test, testuserX, ...)\r
pamtester: performing operation - authenticate\r
Password: "
send -- "badpass\r"
expect "pamtester: Authentication failure\r"
expect eof
_EOT