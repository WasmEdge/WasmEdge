# WasmEdge Runtime Governance

As a sandbox project hosted by CNCF, the WasmEdge Runtime follows the [CNCF Code of Conduct](https://github.com/cncf/foundation/blob/master/code-of-conduct.md).

## Maintainer responsibilities

* Monitor email aliases.
* Monitor Slack (delayed response is perfectly acceptable).
* Triage GitHub issues and perform pull request reviews for other maintainers and the community. The areas of specialization listed in OWNERS.md can be used to help with routing an issue/question to the right person.
* Triage build issues - file issues for known flaky builds or bugs, and either fix or find someone to fix any main build breakages.
* During GitHub issue triage, apply all applicable [labels](https://github.com/WasmEdge/WasmEdge/labels) to each new issue. Labels are extremely useful for future issue to follow-up. Which labels to apply is somewhat subjective so just use your best judgment. A few of the most important labels that are not self-explanatory are:
  * good first issue: Mark any issue that can reasonably be accomplished by a new contributor with this label.
  * help wanted: Unless it is immediately obvious that someone is going to work on an issue (and if so assign it), mark it help wanted.
  * question: If it's unclear if an issue is immediately actionable, mark it with the question label. Questions are easy to search for and close out at a later time. Questions can be promoted to other issue types once it's clear they are actionable (at which point the question label should be removed).
* Make sure that ongoing PRs are moving forward at the right pace or closing them.
* Participate when called upon in the security release process. Note that although this should be a rare occurrence, if a serious vulnerability is found, the process may take up to several full days of work to implement. This reality should be taken into account when discussing time commitment obligations with employers.

## Reviewers

A reviewer is a core maintainer within the project. They share in reviewing issues and pull requests and their LGTM counts towards the required LGTM count to merge a code change into the project.
Reviewers are part of the organization but do not have write access. Becoming a reviewer is a core aspect in the journey to becoming a committer.

## Committers

A committer is a core maintainer who is responsible for the overall quality and stewardship of the project. They share the same reviewing responsibilities as reviewers, but are also responsible for upholding the project bylaws as well as participating in project level votes.

Committers are part of the organization with write access to all repositories. Committers are expected to remain actively involved in the project and participate in voting and discussing proposed project-level changes.

## Adding maintainers

Maintainers are first and foremost contributors who have shown they are committed to the long-term success of a project. Contributors wanting to become maintainers are expected to be deeply involved in contributing code, pull request review, and triage of issues in the project for more than three months.

Just contributing does not make you a maintainer, it is about building trust with the current maintainers of the project and being a person that they can depend on and trust to make decisions in the best interest of the project.

Periodically, the existing maintainers curate a list of contributors who have shown regular activity on the project over the prior months. From this list, maintainer candidates are selected and proposed in a maintainers channel.

After a candidate has been informally proposed in the maintainers' channel, the existing maintainers are given seven days to discuss the candidate, raise objections, and show their support. Formal voting takes place on a pull request that adds the contributor to the MAINTAINERS file. Candidates must be approved by 2/3 of the current maintainers by adding their approval or LGTM to the pull request. The reviewer role has the same process but only requires 1/3 of current maintainers.

If a candidate is approved, they will be invited to add their own LGTM or approval to the pull request to acknowledge their agreement. A maintainer will verify the number of votes that have been received and the allotted seven days have passed, then merge the pull request and invite the contributor to the organization and the [private maintainer mailing list](cncf-wasmedge-runtime-maintainers@lists.cncf.io)).

## When does a maintainer lose maintainer status

If a maintainer is no longer interested or cannot perform the maintainer duties listed above, they should volunteer to be moved to emeritus status. In extreme cases this can also occur by a vote of the maintainers per the voting process below.

## Conflict resolution and voting

In general, we prefer that technical issues and maintainer membership are amicably worked out between the persons involved. If a dispute cannot be decided independently, the maintainers can be called in to decide an issue. If the maintainers themselves cannot decide an issue, the issue will be resolved by voting. The voting process is a simple majority in which each maintainer receives one vote.

## Adding new projects to the WasmEdge Runtime GitHub organization

New projects will be added to the WasmEdge organization via GitHub issue discussion in one of the existing projects in the organization. Once sufficient discussion has taken place (~3-5 business days but depending on the volume of conversation), the maintainers of *the project where the issue was opened* (since different projects in the organization may have different maintainers) will decide whether the new project should be added. See the section above on voting if the maintainers cannot easily decide.

## Meetings

Time zones permitting, Maintainers are expected to participate in the public developer meeting, which occurs on the first Tuesday of each month.
* [Public meeting link](https://us06web.zoom.us/j/89156807241?pwd=VHl5VW5BbmY2eUtTYkY0Zm9yUHRRdz09)
* [Public meeting note](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit?usp=sharing)

Maintainers will also have closed meetings in order to discuss security reports or Code of Conduct violations. Such meetings should be scheduled by any Maintainer on receipt of a security issue or CoC report. All current Maintainers must be invited to such closed meetings, except for any Maintainer who is accused of a CoC violation.

## CNCF Resources
Any Maintainer may suggest a request for CNCF resources, either in the [mailing list](cncf-wasmedge-runtime-maintainers@lists.cncf.io), or during a meeting. A simple majority of Maintainers approve the request. The Maintainers may also choose to delegate working with the CNCF to non-Maintainer community members, who will then be added to the CNCF's Maintainer List for that purpose.

### Code of Conduct
Code of Conduct violations by community members will be discussed and resolved on the private Maintainer mailing list. If a Maintainer is directly involved in the report, the Maintainers will instead designate two Maintainers to work with the CNCF Code of Conduct Committee in resolving it.

## Security Response Team

The Maintainers will appoint a Security Response Team to handle security reports. This committee may simply consist of the Maintainer Council themselves. If this responsibility is delegated, the Maintainers will appoint a team of at least two contributors to handle it. The Maintainers will review who is assigned to this at least once a year.

The Security Response Team is responsible for handling all reports of security holes and breaches according to the [security policy](./SECURITY.md).



## Voting

While most business in WasmEdge runtime is conducted by "lazy consensus", periodically the Maintainers may need to vote on specific actions or changes. 

Generally, a vote will happen on A vote can be taken on the developer mailing list(wasmedge@googlegroup.com)  or the private Maintainer mailing list (cncf-wasmedge-runtime-maintainers@lists.cncf.io) for security or conduct matters. Votes may also be taken at the developer meeting. Any Maintainer may demand a vote be taken.

Most votes require a simple majority of all Maintainers to succeed, except where otherwise noted. Two-thirds majority votes mean at least two-thirds of all existing maintainers.


## Modifying this Charter

Changes to this Governance and its supporting documents may be approved by a 2/3 vote of the Maintainers.



## Credits

Sections of this document have been borrowed from [Helm](https://github.com/helm/blob/main/governance/governance.md) and [Envoy](https://github.com/envoyproxy/envoy/blob/master/GOVERNANCE.md)  projects.
