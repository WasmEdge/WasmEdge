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
- [Vendor Neutrality](#vendor-neutrality)
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

* Code of Conduct violations by community members will be discussed and resolved on the private Maintainer mailing list <cncf-wasmedge-runtime-maintainers@lists.cncf.io>. 
* If a Maintainer is directly involved in the report, the Maintainers will instead designate two Maintainers to work with the CNCF Code of Conduct Committee <conduct@cncf.io> in resolving it.

See more details in [Code of Conduct](CODE_OF_CONDUCT.md) document.

## Security Response Team

The Maintainers will appoint a Security Response Team to handle security reports. This committee may simply consist of the Maintainer Council themselves. If this responsibility is delegated, the Maintainers will appoint a team of at least two contributors to handle it. The Maintainers will review who is assigned to this at least once a year.

The Security Response Team is responsible for handling all reports of security holes and breaches according to the [security policy](./SECURITY.md).


## Voting

While most business in WasmEdge runtime is conducted by "lazy consensus", periodically the Maintainers may need to vote on specific actions or changes.

Generally, a vote can be taken on the developer mailing list (wasmedge@googlegroups.com) or the private Maintainer mailing list (cncf-wasmedge-runtime-maintainers@lists.cncf.io) for security or conduct matters, or via GitHub.

### Voting Requirements

**Simple majority (>1/2)** of all Maintainers is required for:
* Promoting or removing reviewers
* General conflict resolution
  

**Two-thirds majority (≥2/3)** of all Maintainers is required for:
* Creating new repositories or subprojects
* Changing documentation in `WasmEdge/docs` folder, including governance documents
* Promoting or removing committers or maintainers

Any Maintainer may demand a vote be taken.

### Voting Process
- **Timeline**: Votes remain open for minimum 5 days for two-thirds majority votes, 3 days for simple majority votes
- **Quorum**: No minimum required; based on all existing Maintainers
- **Documentation**: All votes and outcomes must be recorded
- **Tie-breaking**: Tied votes fail

## Vendor Neutrality
### Current Status
Currently, all maintainers are from Second State. However, we have reviewers and committers from different organizations. We recognize the importance of vendor neutrality for building a healthy open source community and are actively working to achieve it.

### Our Plan
- Identify, engage and promote qualified contributors from other organizations, particularly from our existing reviewers and committers
- **Target**: No single organization controls more than 50% of maintainer seats

### Interim Measures
While building our diverse maintainer base:
- All major decisions require public discussion and are easy for the community to participate in. For example, roadmap changes require creating a public GitHub issue where community members can discuss proposed changes before implementation.
- We actively seek feedback from users and contributors outside our organization
- Governance processes are transparent and documented

### Accountability
We will provide quarterly updates on our progress toward vendor neutrality and welcome community feedback on our approach.

## Modifying this Charter
Changes to this Governance and its supporting documents may be approved by a two-thirds majority (≥2/3) vote of the Maintainers.

## Credits
WasmEdge adopts [CNCF governance template](https://contribute.cncf.io/maintainers/templates/governance-maintainer/) as the governance model.
