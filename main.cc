#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#include <v8/include/v8.h>
#include <v8/include/libplatform/libplatform.h>
#include <v8/src/utils/version.h>
#include <v8/src/snapshot/snapshot-data.h>
#include <v8/src/snapshot/code-serializer.h>
#include <v8/src/handles/handles.h>
#include <v8/src/objects/shared-function-info-inl.h>
#include <v8/src/objects/string.h>
#include <v8/src/objects/code.h>
#include <v8/src/base/memory.h>
#include <v8/src/base/vector.h>

namespace v8b = v8::base;
namespace v8i = v8::internal;

void PrintSharedFunctionInfoAsJSON(v8i::Handle<v8i::SharedFunctionInfo> shared_func_info, int depth = 0) {

}

void PrintBytecodeArrayAsJSON(std::ostream& out, v8i::Handle<v8i::BytecodeArray> bytecode_array, int depth = 0) {
	out << "{" << std::endl;
	out << std::string(depth + 1, '\t') << "\"frame_size\": " << bytecode_array->frame_size() << "," << std::endl;
	out << std::string(depth + 1, '\t') << "\"register_count\": " << bytecode_array->register_count() << "," << std::endl;
	out << std::string(depth + 1, '\t') << "\"parameter_count\": " << bytecode_array->parameter_count() << "," << std::endl;

	out << std::string(depth + 1, '\t') << "\"source_positions\": [" << std::endl;
	v8i::SourcePositionTableIterator source_positions(
		bytecode_array->SourcePositionTable(),
		v8i::SourcePositionTableIterator::kAll,
		v8i::SourcePositionTableIterator::kDontSkipFunctionEntry);
	for (; !source_positions.done(); source_positions.Advance()) {
		out << std::string(depth + 2, '\t') << "{" << std::endl;
		out << std::string(depth + 3, '\t') << "\"code_offset\": " << source_positions.code_offset() << "," << std::endl;
		out << std::string(depth + 3, '\t') << "\"source_position\": " << source_positions.source_position() << "," << std::endl;
		out << std::string(depth + 3, '\t') << "\"is_statement\": " << source_positions.is_statement() << "," << std::endl;
		out << std::string(depth + 2, '\t') << "}" << std::endl;
	}
	out << std::string(depth + 1, '\t') << "]" << std::endl;

	out << std::string(depth, '\t') << "}" << std::endl;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		return 1;
	}

	std::ifstream file(argv[1], std::ios::in | std::ios::binary);

	if (!file) {
		throw errno;
	}
	std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
	v8::V8::InitializePlatform(platform.get());
	v8::V8::Initialize();

	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	v8::Isolate* isolate = v8::Isolate::New(create_params);
	auto internal_isolate = reinterpret_cast<v8i::Isolate*>(isolate);

	{
		v8::Isolate::Scope isolate_scope(isolate);
		v8::HandleScope handle_scope(isolate);

		/*		
		uint32_t magic_number = v8i::SerializedData::kMagicNumber;
		buffer[v8i::SerializedData::kMagicNumberOffset + 0] = (magic_number >> 0) & 0xFF;
		buffer[v8i::SerializedData::kMagicNumberOffset + 1] = (magic_number >> 8) & 0xFF;
		buffer[v8i::SerializedData::kMagicNumberOffset + 2] = (magic_number >> 16) & 0xFF;
		buffer[v8i::SerializedData::kMagicNumberOffset + 3] = (magic_number >> 24) & 0xFF;
		*/

		uint32_t version_hash = v8i::Version::Hash();
		buffer[v8i::SerializedCodeData::kVersionHashOffset + 0] = (version_hash >> 0) & 0xFF;
		buffer[v8i::SerializedCodeData::kVersionHashOffset + 1] = (version_hash >> 8) & 0xFF;
		buffer[v8i::SerializedCodeData::kVersionHashOffset + 2] = (version_hash >> 16) & 0xFF;
		buffer[v8i::SerializedCodeData::kVersionHashOffset + 3] = (version_hash >> 24) & 0xFF;

		uint32_t flags_hash = v8i::FlagList::Hash();
		buffer[v8i::SerializedCodeData::kFlagHashOffset + 0] = (flags_hash >> 0) & 0xFF;
		buffer[v8i::SerializedCodeData::kFlagHashOffset + 1] = (flags_hash >> 8) & 0xFF;
		buffer[v8i::SerializedCodeData::kFlagHashOffset + 2] = (flags_hash >> 16) & 0xFF;
		buffer[v8i::SerializedCodeData::kFlagHashOffset + 3] = (flags_hash >> 24) & 0xFF;

		auto aligned_cache_data = std::make_unique<v8i::AlignedCachedData>(buffer.data(), buffer.size());

		auto source_hash = v8b::ReadLittleEndianValue<uint32_t>(
				reinterpret_cast<v8b::Address>(buffer.data()) + v8i::SerializedCodeData::kSourceHashOffset);
		std::string fake_source_str(source_hash, ' ');
		auto fake_source = internal_isolate->factory()->NewStringFromUtf8(v8b::CStrVector(fake_source_str.c_str())).ToHandleChecked();

		auto origin_options = v8::ScriptOriginOptions(
				true,
				false,
				false,
				false);

		auto maybe_shared_func_info = v8i::CodeSerializer::Deserialize(
				internal_isolate,
				aligned_cache_data.get(),
				fake_source,
				origin_options);

		if (maybe_shared_func_info.is_null()) {
			auto sanity_check_result = v8i::SerializedCodeSanityCheckResult::kSuccess;
			auto scd = v8i::SerializedCodeData::FromCachedData(
					aligned_cache_data.get(),
					v8i::SerializedCodeData::SourceHash(fake_source, origin_options),
					&sanity_check_result
					);
			switch (sanity_check_result) {
				case v8i::SerializedCodeSanityCheckResult::kMagicNumberMismatch:
					std::cout << "Magic number mismatch" << std::endl;
					std::cout << std::hex << v8i::SerializedCodeData::kMagicNumber << std::endl;
					std::cout << std::hex << ((buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0]) << std::endl;
					break;
				case v8i::SerializedCodeSanityCheckResult::kVersionMismatch:
					std::cout << "Version mismatch" << std::endl;
					break;
				case v8i::SerializedCodeSanityCheckResult::kSourceMismatch:
					std::cout << "Source hash mismatch" << std::endl;
					break;
				case v8i::SerializedCodeSanityCheckResult::kFlagsMismatch:
					std::cout << "Flags mismatch" << std::endl;
					std::cout << std::hex << v8i::FlagList::Hash() << std::endl;
					std::cout << std::hex << ((buffer[15] << 24) | (buffer[14] << 16) | (buffer[13] << 8) | buffer[12]) << std::endl;
					break;
				case v8i::SerializedCodeSanityCheckResult::kChecksumMismatch:
					std::cout << "Checksum fail";
					break;
				case v8i::SerializedCodeSanityCheckResult::kInvalidHeader:
					std::cout << "Invalid header";
					break;
				case v8i::SerializedCodeSanityCheckResult::kLengthMismatch:
					std::cout << "Length mismatch";
					break;
			}
			return 1;
		}

		auto shared_func_info = maybe_shared_func_info.ToHandleChecked();

		auto bytecode_array = shared_func_info->GetBytecodeArray(internal_isolate);
		auto bytecode_array_handle = v8i::handle(bytecode_array, internal_isolate);
		PrintBytecodeArrayAsJSON(std::cout, bytecode_array_handle);
		bytecode_array.Disassemble(std::cout);
		bytecode_array.PrintJson(std::cout);
	}

	isolate->Dispose();
	v8::V8::Dispose();
	v8::V8::DisposePlatform();
	delete create_params.array_buffer_allocator;

	return 0;
}
