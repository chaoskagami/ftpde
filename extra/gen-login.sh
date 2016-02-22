#!/bin/bash

>&2 echo "Generating entry to insert to config."
>&2 read -p "Username: " user
>&2 read -s -p "Password: " pass
>&2 echo ''

dd if=/dev/urandom bs=1 count=16 of=salt.bin 2>/dev/null
cp salt.bin password_file
echo -n "$pass" >> password_file
salt_hash="$(sha256sum -b password_file | sed 's| .*$||')"
salt_hex="$(xxd -p salt.bin | tr -d '\n')"

rm salt.bin password_file
echo "${user} = \"sha256:${salt_hex}:${salt_hash}\""
