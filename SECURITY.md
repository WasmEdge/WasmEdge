# Security policy

## Security bulletins

For information regarding the security of WasmEdge please join:

* Mailing List <wasmedge@googlegroup.com>

## Reporting a vulnerability

Please use the below process to report a vulnerability to WasmEdge:

Email:

1. Send email to <wasmedge-security@lists.cncf.io>
    * Emails should contain:
        * description of the problem
        * precise and detailed steps (include screenshots) that created the
          problem
        * the affected version(s)
        * any possible mitigations, if known
1. You will receive a reply from one of the maintainers within 24 hours
   acknowledging receipt of the email. After that, we will give a detailed
   response about the subsequent process within 48 hours.
1. Please do not submit security vulnerabilities directly as Github Issues.

Web:

1. Please visit [GitHub Seuciry Advisory of WasmEdge](https://github.com/WasmEdge/WasmEdge/security/advisories/new)
   * You will receive a confirmation email upon submission

WasmEdge follows a **`90 days` disclosure timeline**. Refer to our [embargo policy](./docs/embargo-policy.md) for more information.

## Disclosure policy

For known public security vulnerabilities, we will disclose the disclosure as soon as possible after receiving the report. Vulnerabilities discovered for the first time will be disclosed in accordance with the following process:

* The received security vulnerability report shall be handed over to the security team for follow-up coordination and repair work.
* After the vulnerability is confirmed, we will create a draft Security Advisory on Github that lists the details of the vulnerability.
* Invite related personnel to discuss about the fix.
* Fork the temporary private repository on Github, and collaborate to fix the vulnerability.
* After the fix code is merged into all supported versions, the vulnerability will be publicly posted in the GitHub Advisory Database.

## Supported Versions

Information regarding supported versions of WasmEdge are in the below table:

| Version | Supported |
| ------- | --------- |
| 0.14.0  | :white_check_mark: |
| 0.13.5  | :white_check_mark: |
