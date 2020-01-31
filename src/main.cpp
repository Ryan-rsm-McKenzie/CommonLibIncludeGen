#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>
#include <vector>

#include <string.h>


class PathSorter
{
public:
	bool operator()(const std::filesystem::path& a_lhs, std::filesystem::path& a_rhs) const
	{
		auto lhsNum = GetNumElements(a_lhs);
		auto rhsNum = GetNumElements(a_rhs);
		if (lhsNum != rhsNum) {
			return lhsNum < rhsNum;
		}

		auto lhsIt = a_lhs.begin();
		auto rhsIt = a_rhs.begin();
		while (lhsIt != a_lhs.end() && rhsIt != a_rhs.end()) {
			auto comp = CICompare(lhsIt->c_str(), rhsIt->c_str());
			if (comp != 0) {
				return comp < 0;
			}

			lhsIt++;
			rhsIt++;
		}

		return false;
	}

private:
	std::size_t GetNumElements(const std::filesystem::path& a_path) const
	{
		std::size_t num = 0;
		for ([[maybe_unused]] auto& elem : a_path) {
			++num;
		}
		return num;
	}


	int CICompare(const char* a_lhs, const char* a_rhs) const
	{
		return _stricmp(a_lhs, a_rhs);
	}


	int CICompare(const wchar_t* a_lhs, const wchar_t* a_rhs) const
	{
		return _wcsicmp(a_lhs, a_rhs);
	}
};


std::pair<std::filesystem::path, std::filesystem::path> GetPaths(const char* a_invokeName)
{
	std::error_code ec;
	std::filesystem::path rootPath(a_invokeName);
	rootPath = rootPath.parent_path();
	rootPath = rootPath.lexically_normal();
	if (!std::filesystem::exists(rootPath, ec) || ec) {
		std::cerr << "Current path does not exist!";
		std::exit(EXIT_FAILURE);
	}

	auto includePath = rootPath;
	includePath /= "include";
	includePath = includePath.lexically_normal();
	if (!std::filesystem::exists(includePath, ec) || ec) {
		std::cerr << "Include path does not exist!";
		std::exit(EXIT_FAILURE);
	}

	auto headerPath = includePath;
	headerPath /= "RE";
	headerPath = headerPath.lexically_normal();
	if (!std::filesystem::exists(headerPath, ec) || ec) {
		std::cerr << "Include path does not exist!";
		std::exit(EXIT_FAILURE);
	}

	return std::make_pair(includePath, headerPath);
}


// https://en.cppreference.com/w/cpp/algorithm/lower_bound
template <class ForwardIt, class T, class Compare = std::less<>>
ForwardIt binary_find(ForwardIt a_first, ForwardIt a_last, const T& a_value, Compare a_comp = {})
{
	a_first = std::lower_bound(a_first, a_last, a_value, a_comp);
	return a_first != a_last && !a_comp(a_value, *a_first) ? a_first : a_last;
}


int main([[maybe_unused]] int a_argc, [[maybe_unused]] char* a_argv[])
{
	using FileArray = std::vector<std::filesystem::path>;

	if (a_argc < 1) {
		std::cerr << "Not enough arguments were passed to evaluate the current path!";
		return EXIT_FAILURE;
	}

	const std::filesystem::path EXT_FILTER(".h");
	auto [includePath, headerPath] = GetPaths(a_argv[0]);
	FileArray files;

	for (auto& entry : std::filesystem::recursive_directory_iterator(headerPath)) {
		if (entry.is_regular_file()) {
			auto& path = entry.path();
			if (path.has_extension() && path.extension() == EXT_FILTER) {
				std::error_code ec;
				auto file = std::filesystem::relative(path, includePath, ec);
				if (ec) {
					std::cerr << "An error occurred while building a relative path!";
					return EXIT_FAILURE;
				}

				files.push_back(file.lexically_normal());
			}
		}
	}

	auto outFilePath = headerPath;
	outFilePath /= "Skyrim.h";
	std::ofstream outFile(outFilePath);
	if (!outFile.is_open()) {
		std::cerr << "Failed to open output file!";
		return EXIT_FAILURE;
	}

	std::sort(files.begin(), files.end(), PathSorter());

	// prevent self include
	std::error_code ec;
	const auto FILE_FILTER = std::filesystem::relative(outFilePath, includePath, ec);
	if (ec) {
		std::cerr << "An error occurred while building a relative path!";
		return EXIT_FAILURE;
	} else {
		auto it = binary_find(files.begin(), files.end(), FILE_FILTER);
		if (it != files.end()) {
			files.erase(it);
		}
	}

	outFile << "#pragma once" << '\n';
	outFile << '\n';
	for (auto& file : files) {
		outFile << "#include \"" << file.generic_u8string() << '\"' << '\n';
	}

	return EXIT_SUCCESS;
}
