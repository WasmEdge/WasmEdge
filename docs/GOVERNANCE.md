# WasmEdge Runtime Governance

As a sandbox project hosted by CNCF, the WasmEdge Runtime follows the [CNCF Code of Conduct](https://github.com/cncf/foundation/blob/master/code-of-conduct.md).

- [Values](#values)
- [Maintainers Structure](#maintainers-structure)
  - [Maintainers](#maintainers-responsibilities)
  - [Reviewers](#reviewers)
  - [Committers](#committers)
  - [Adding maintainers](#adding-maintainers)
  - [When does a maintainer lose maintainer status](#when-does-a-maintainer-lose-maintainer-status)
  - [Conflict resolution and voting](#conflict-resolution-and-voting)
- [Meetings](#meetings)
- [CNCF Resources](#cncf-resources)
- [Code of Conduct Enforcement](#code-of-conduct)
- [Security Response Team](#security-response-team)
- [Voting](#voting)
- [Modifications](#modifying-this-charter)
- [Credits](#credits)

The WasmEdge project and its leadership embrace the following values:

* Openness: Communication and decision-making happens in the open and is discoverable for future
  reference. As much as possible, all discussions and work take place in public
  forums and open repositories.

* Fairness: All stakeholders have the opportunity to provide feedback and submit
  contributions, which will be considered on their merits.

* Community over Product or Company: Sustaining and growing our community takes
  priority over shipping code or sponsors' organizational goals.  Each
  contributor participates in the project as an individual.

* Inclusivity: We innovate through different perspectives and skill sets, which
  can only be accomplished in a welcoming and respectful environment.

* Participation: Responsibilities within the project are earned through
  participation, and there is a clear path up the contributor ladder into leadership
  positions.

## Maintainers Structure

There are three levels of maintainers for WasmEdge. The WasmEdge maintainers oversee the overall
project and its health. Committers focus on a single codebase, a group of related
codebases, a service (e.g., a website), or project to support the other projects (e.g., marketing or
community management). Reviewers help review the GitHub issues and PRs. See the [Contributor Ladder](./CONTRIBUTION_LADDER.md) for more detailed information on responsibilities.

### Maintainers

WasmEdge Maintainers have write access to the [WasmEdge](https://github.com/WasmEdge/WasmEdge) GitHub repo.
They can merge their own patches or patches from others. The current maintainers
can be found in [MAINTAINERS.md](./OWNERS.md).  Maintainers collectively manage the project's
resources and contributors.

This privilege is granted with some expectation of responsibility: maintainers are people who care about the WasmEdge project and want to help it grow and improve. A maintainer is not just someone who can make changes, but someone who has demonstrated their ability to collaborate with the team, get the most knowledgeable people to review code and docs, contribute high-quality code, and follow through to fix issues (in code or tests).

A maintainer is a contributor to the project's success and a citizen helping the project succeed.

The collective team of all Maintainers is known as the Maintainer Council, which is the governing body for the project.


### Reviewers

A Reviewer is an established contributor who regularly participates in the project. Reviewers have privileges in both project repositories and elections, and as such are expected to act in the interests of the whole project. Becoming a reviewer is a core aspect in the journey to becoming a committer.

### Committers

A Committer has responsibility for specific code, documentation, test, or other project areas. They are collectively responsible, with other Committers, for reviewing all changes to those areas and indicating whether those changes are ready to merge. They have a track record of contribution and review in the project.

Committers are part of the organization with write access to all repositories. Committers are expected to remain actively involved in the project and participate in voting and discussing proposed project-level changes.

### Adding maintainers

Maintainers are first and foremost contributors who have shown they are committed to the long-term success of a project. Contributors wanting to become maintainers are expected to be deeply involved in contributing code, pull request review, and triage of issues in the project for more than three months.

Just contributing does not make you a maintainer, it is about building trust with the current maintainers of the project and being a person that they can depend on and trust to make decisions in the best interest of the project.

Periodically, the existing maintainers curate a list of contributors who have shown regular activity on the project over the prior months. From this list, maintainer candidates are selected and proposed in a maintainers channel.

After a candidate has been informally proposed in the maintainers' channel, the existing maintainers are given seven days to discuss the candidate, raise objections, and show their support. Formal voting takes place on a pull request that adds the contributor to the MAINTAINERS file. Candidates must be approved by 2/3 of the current maintainers by adding their approval or LGTM to the pull request. The reviewer role has the same process but only requires 1/3 of current maintainers.

If a candidate is approved, they will be invited to add their own LGTM or approval to the pull request to acknowledge their agreement. A maintainer will verify the number of votes that have been received and the allotted seven days have passed, then merge the pull request and invite the contributor to the organization and the [private maintainer mailing list](cncf-wasmedge-runtime-maintainers@lists.cncf.io)).

### When does a maintainer lose maintainer status

If a maintainer is no longer interested or cannot perform the maintainer duties listed above, they should volunteer to be moved to emeritus status. In extreme cases this can also occur by a vote of the maintainers per the voting process below.

### Conflict resolution and voting

In general, we prefer that technical issues and maintainer membership are amicably worked out between the persons involved. If a dispute cannot be decided independently, the maintainers can be called in to decide an issue. If the maintainers themselves cannot decide an issue, the issue will be resolved by voting. The voting process is a simple majority in which each maintainer receives one vote.

## Adding new projects

New projects will be added to the WasmEdge organization via GitHub issue discussion in one of the existing projects in the organization. Once sufficient discussion has taken place (~3-5 business days but depending on the volume of conversation), the maintainers of *the project where the issue was opened* (since different projects in the organization may have different maintainers) will decide whether the new project should be added. See the section above on voting if the maintainers cannot easily decide.

## Meetings

Time zones permitting, Maintainers are expected to participate in the public developer meeting, which occurs on the first Tuesday of each month.
* [Public meeting link](https://us06web.zoom.us/j/89156807241?pwd=VHl5VW5BbmY2eUtTYkY0Zm9yUHRRdz09)
* [Public meeting note](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit?usp=sharing)

Maintainers will also have closed meetings in order to discuss security reports or Code of Conduct violations. Such meetings should be scheduled by any Maintainer on receipt of a security issue or CoC report. All current Maintainers must be invited to such closed meetings, except for any Maintainer who is accused of a CoC violation.

## CNCF Resources
Any Maintainer may suggest a request for CNCF resources, either in the [mailing list](cncf-wasmedge-runtime-maintainers@lists.cncf.io), or during a meeting. A simple majority of Maintainers approve the request. The Maintainers may also choose to delegate working with the CNCF to non-Maintainer community members, who will then be added to the CNCF's Maintainer List for that purpose.

## Code of Conduct
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

WasmEdge adopts [CNCF governance template](https://contribute.cncf.io/maintainers/templates/governance-maintainer/) as the governance model.
