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

Maintainers are first and foremost contributors that have shown they are committed to the long term success of a project. Contributors wanting to become maintainers are expected to be deeply involved in contributing code, pull request review, and triage of issues in the project for more than three months.

Just contributing does not make you a maintainer, it is about building trust with the current maintainers of the project and being a person that they can depend on and trust to make decisions in the best interest of the project.

Periodically, the existing maintainers curate a list of contributors that have shown regular activity on the project over the prior months. From this list, maintainer candidates are selected and proposed in the maintainers forum.

After a candidate has been informally proposed in the maintainers forum, the existing maintainers are given seven days to discuss the candidate, raise objections and show their support. Formal voting takes place on a pull request that adds the contributor to the MAINTAINERS file. Candidates must be approved by 2/3 of the current committers by adding their approval or LGTM to the pull request. The reviewer role has the same process but only requires 1/3 of current committers.

If a candidate is approved, they will be invited to add their own LGTM or approval to the pull request to acknowledge their agreement. A committer will verify the numbers of votes that have been received and the allotted seven days have passed, then merge the pull request and invite the contributor to the organization.

## When does a maintainer lose maintainer status

If a maintainer is no longer interested or cannot perform the maintainer duties listed above, they should volunteer to be moved to emeritus status. In extreme cases this can also occur by a vote of the maintainers per the voting process below.

## Conflict resolution and voting

In general, we prefer that technical issues and maintainer membership are amicably worked out between the persons involved. If a dispute cannot be decided independently, the maintainers can be called in to decide an issue. If the maintainers themselves cannot decide an issue, the issue will be resolved by voting. The voting process is a simple majority in which each maintainer receives one vote.

## Adding new projects to the WasmRuntime GitHub organization

New projects will be added to the WasmEdge organization via GitHub issue discussion in one of the existing projects in the organization. Once sufficient discussion has taken place (~3-5 business days but depending on the volume of conversation), the maintainers of *the project where the issue was opened* (since different projects in the organization may have different maintainers) will decide whether the new project should be added. See the section above on voting if the maintainers cannot easily decide.

## DCO and Licenses

The following licenses and contributor agreements will be used for WasmRuntime projects:

* [Apache 2.0](https://opensource.org/licenses/Apache-2.0) for code
* [Developer Certificate of Origin](https://developercertificate.org/) for new contributions

## Credits

Sections of this document have been borrowed from [Helm](https://github.com/helm/blob/main/governance/governance.md) and [Envoy](https://github.com/envoyproxy/envoy/blob/master/GOVERNANCE.md)  projects.
