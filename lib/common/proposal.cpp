#include "common/proposal.h"
namespace SSVM {

using namespace std::literals::string_view_literals;
/// Proposal name enumeration string mapping.
const std::unordered_map<Proposal, std::string_view> ProposalStr = {
    {Proposal::Annotations, "annotations"sv},
    {Proposal::BulkMemoryOperations, "bulk-memory-operations"sv},
    {Proposal::ExceptionHandling, "exception-handling"sv},
    {Proposal::FunctionReferences, "function-references"sv},
    {Proposal::Memory64, "memory64"sv},
    {Proposal::ReferenceTypes, "reference-types"sv},
    {Proposal::SIMD, "simd"sv},
    {Proposal::TailCall, "tail-call"sv},
    {Proposal::Threads, "threads"sv}};

} // namespace SSVM
