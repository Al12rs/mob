#include "pch.h"
#include "tasks.h"

namespace mob
{

lz4::lz4()
	: basic_task("lz4")
{
}

fs::path lz4::source_path()
{
	return paths::build() / ("lz4-" + versions::lz4());
}

void lz4::do_clean_for_rebuild()
{
	op::delete_directory(cx(), solution_dir() / "bin", op::optional);
}

void lz4::do_fetch()
{
	run_tool(git_clone()
		.url(make_github_url("lz4","lz4"))
		.branch(versions::lz4())
		.output(source_path()));

	run_tool(devenv_upgrade(solution_file()));
}

void lz4::do_build_and_install()
{
	run_tool(msbuild()
		.solution(solution_file())
		.projects({"liblz4-dll"}));

	op::copy_file_to_dir_if_better(cx(),
		out_dir() / "liblz4.dll",
		paths::install_dlls());

	op::copy_file_to_dir_if_better(cx(),
		out_dir() / "liblz4.pdb",
		paths::install_pdbs());
}

fs::path lz4::solution_dir()
{
	return source_path() / "visual" / "VS2017";
}

fs::path lz4::solution_file()
{
	return solution_dir() / "lz4.sln";
}

fs::path lz4::out_dir()
{
	return solution_dir() / "bin" / "x64_Release";
}

}	// namespace