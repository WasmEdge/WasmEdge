# Security policy

## Security bulletins

For information regarding the security of this project please join:

* [Public] Chat on the #wasmedge channel at the CNCF Slack
* [Private] Send an email to the security contact: dm4@secondstate.io.
* [Public] Open an issue on GitHub

You can also submit a fix to the issue by forking the affected repository and sending us a pull request. However, we prefer you'd talk to us first, as our repositories are public and we would like to give a heads-up to our users before disclosing vulnerabilities publicly.

## Reporting a vulnerability

Please use the below process to report a vulnerability to the project:

### Email:

1. Email the **security contact**: **dm4@secondstate.io**
    * Emails should contain:
        * description of the problem
        * precise and detailed steps (including screenshots) that created the
          problem
        * the affected version(s)
        * any possible mitigations, if known
1. You will receive a reply from one of the maintainers within **1 day** acknowledging receipt of the email.
2. You may be contacted by the email **dm4@secondstate.io** to further discuss the reported item.
   Please bear with us as we seek to understand the breadth and scope of the
   reported problem, recreate it, and confirm if there is a vulnerability
   present.


### Slack and GitHub issue:

If you choose a public channel to communicate with us, please encrypt your message using our public key ( To be added). It is available in all major key servers and should match the one shown below.

If you are new to PGP, you can run the following command to encrypt a file called "message.txt":
```
# Receive our keys from a key server:
gpg --keyserver keyserver.ubuntu.com --recv-keys C043A4D2B3F2AC31

# Alternatively, copy the key below to file C043A4D2B3F2AC31.asc and import it:
gpg --import C043A4D2B3F2AC31.asc

# Encrypt a "message.txt" file into "message.txt.asc":
gpg -ea -r C043A4D2B3F2AC31 message.txt

# Send us the resulting "message.txt.asc"
```
#### Our public key

To be Added

This project follows a **5 workdays** disclosure timeline**. Refer to our [embargo
policy](#embargo-policy) for more information.

## Supported Versions

The WasmEdge Runtime project provides community support only for last minor version: bug fixes are released either as part of the next minor version or as an on-demand patch version. Independent of which version is next, all patch versions are cumulative, meaning that they represent the state of our main branch at the moment of the release. For instance, if the latest version is 0.13.0, bug fixes are released either as part of 0.14.0 or 0.13.1.

Security fixes are given priority and might be enough to cause a new version to be released.

## Embargo Policy

This policy forbids members of this project's security contacts (dm4@secondstate.io) and others
defined below from sharing information outside of the security contacts and this
listing without need-to-know and advance notice.

The information members and others receive from the list defined below must:

* not be made public,
* not be shared,
* not be hinted at
* must be kept confidential and close held

Except with the list's explicit approval. This holds true until the public
disclosure date/time that was agreed upon by the list.

If information is inadvertently shared beyond what is allowed by this policy,
you are REQUIRED to inform the security contacts (dm4@secondstate.io) of exactly what
information leaked and to whom. A retrospective will take place after the leak
so we can assess how to not make this mistake in the future.

Violation of this policy will result in the immediate removal and subsequent
replacement of you from this list or the Security Contacts.

### Disclosure Timeline

This project sustains a **5 workdays disclosure timeline** to ensure we provide a
quality, tested release. On some occasions, we may need to extend this timeline
due to the complexity of the problem, lack of expertise available, or other reasons.
Submitters will be notified if an extension occurs.
