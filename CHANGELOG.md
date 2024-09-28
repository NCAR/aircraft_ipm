# Changelog

Changelog for iPM code

## [Unreleased]

## [0.1] - 2023-09-10 - First tagged release

- iPM control program 
- simple iPM emulator
- jenkinsfiles for continuous build on CentOS8 and UbuntuBionic
- License
- Documentation
- debian package builder

## [0.2] - 2024-09-27 - Updates to options and return codes

- Move register configuration to a command line option requiring elevated privs
- Update BITRESULT? to print response data as a UDP string of float or hex.
- Update SERNO? and VER? to print return value rather than success/failure
- Add return codes
- Clean up verbose mode for nonprinting chars
- Change -d debug to -H hexidecimal
- Change -p port to -D device
- Clarify meaning of procqueries and port in Usage statement
- Add typical examples to Usage statement

- Restructure code; naiipm class becoming unweildly; split into logical classes
- Break up unit tests to mirror restucturing
- Default packets sent to nidas are now hex. The ipm.xml file has been updated to apply scaling during processing.
