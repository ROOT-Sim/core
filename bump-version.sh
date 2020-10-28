#!/bin/bash

describe=$(git describe)
branchname=$(git branch 2> /dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/\1/')

# Split version in major, minor, hotfix
IFS='-' read version garbage <<< "$describe"
IFS='.' read major minor hotfix <<< "$version"

# Get the type of branch, depending on the name
IFS='-' read branchtype garbage <<< "$branchname"

echo "The current branch ($branchname) is of type $branchtype"
echo "Current version is $major.$minor.$hotfix"

# Check if we have to increment the major
if [ "$1" == "major" ]; then
	major=$((major + 1))
	minor=0
	hotfix=0
elif [ "$branchtype" == "hotfix" ]; then
	hotfix=$((hotfix + 1))
elif [ "$branchtype" == "release" ]; then
	minor=$((minor + 1))
else
	echo "Branch name doesn't tell the version should be bumped."
	exit 1
fi

echo "Applying new version number: $major.$minor.$hotfix. Continue? (y/n)"
old_stty_cfg=$(stty -g)
stty raw -echo ; answer=$(head -c 1) ; stty $old_stty_cfg # Careful playing with stty
if echo "$answer" | grep -iq "^y" ;then
	echo "Applying changes to meson.build"
else
	exit 0
fi

# Update version in meson.build
sed -i "s/version: \o047.*\o047/version: \o047$major.$minor.$hotfix\o047/" meson.build


echo "Files modified successfully, version bumped to $major.$minor.$hotfix"
