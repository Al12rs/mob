#include "pch.h"
#include "tasks.h"

namespace mob
{

sip::sip()
	: basic_task("sip")
{
}

std::string sip::version()
{
	return conf::version_by_name("sip");
}

std::string sip::version_for_pyqt()
{
	return conf::version_by_name("pyqt_sip");
}

bool sip::prebuilt()
{
	return false;
}

fs::path sip::source_path()
{
	return paths::build() / ("sip-" + version());
}

fs::path sip::sip_module_exe()
{
	return python::scripts_path() / "sip-module.exe";
}

fs::path sip::sip_install_exe()
{
	return python::scripts_path() / "sip-install.exe";
}

fs::path sip::module_source_path()
{
	// 12.7.2
	// .2 is optional
	std::regex re(R"((\d+)\.(\d+)(?:\.(\d+))?)");
	std::smatch m;

	const auto s = version_for_pyqt();

	if (!std::regex_match(s, m, re))
		bail_out("bad pyqt sip version {}", s);

	// 12.7
	const auto dir = m[1].str() + "." + m[2].str();

	return source_path() / "sipbuild" / "module" / "source" / dir;
}

void sip::do_clean(clean c)
{
	instrument<times::clean>([&]
	{
		if (is_set(c, clean::reextract))
		{
			cx().trace(context::reextract, "deleting {}", source_path());
			op::delete_directory(cx(), source_path(), op::optional);
			return;
		}

		if (is_set(c, clean::rebuild))
			op::delete_directory(cx(), source_path() / "build", op::optional);
	});
}

void sip::do_fetch()
{
	// downloading uses python.exe and so has to wait until it's built
}

void sip::do_build_and_install()
{
	instrument<times::fetch>([&]
	{
		download();
	});

	instrument<times::extract>([&]
	{
		run_tool(extractor()
			.file(download_file())
			.output(source_path()));
	});

	instrument<times::build>([&]
	{
		generate();
	});

	op::copy_file_to_dir_if_better(cx(),
		source_path() / "sip.h",
		python::include_path());
}

void sip::download()
{
	if (fs::exists(download_file()))
	{
		if (conf::redownload())
		{
			cx().trace(context::redownload, "deleting {}", download_file());
			op::delete_file(cx(), download_file(), op::optional);
		}
		else
		{
			cx().trace(context::bypass,
				"sip: {} already exists", download_file());

			return;
		}
	}

	run_tool(process_runner(process()
		.binary(python::python_exe())
		.chcp(65001)
		.stdout_encoding(encodings::utf8)
		.stderr_encoding(encodings::utf8)
		.arg("-X", "utf8")
		.arg("-m", "pip")
		.arg("download")
		.arg("--no-binary=:all:")
		.arg("--no-deps")
		.arg("-d", paths::cache())
		.arg("sip==" + version())
		.env(this_env::get()
			.set("PYTHONUTF8", "1"))));
}

void sip::generate()
{
	const auto header = source_path() / "sip.h";

	if (fs::exists(header))
	{
		if (conf::rebuild())
		{
			cx().trace(context::rebuild, "ignoring {}", header);
		}
		else
		{
			cx().trace(context::bypass, "{} already exists", header);
			return;
		}
	}

	run_tool(process_runner(process()
		.binary(python::python_exe())
		.chcp(65001)
		.stdout_encoding(encodings::utf8)
		.stderr_encoding(encodings::utf8)
		.stderr_filter([&](process::filter& f)
		{
			if (f.line.find("zip_safe flag not set") != std::string::npos)
				f.lv = context::level::trace;
			else if (f.line.find("module references __file__") != std::string::npos)
				f.lv = context::level::trace;
		})
		.arg("-X", "utf8")
		.arg("setup.py")
		.arg("install")
		.cwd(source_path())
		.env(this_env::get()
			.set("PYTHONUTF8", "1"))));


	const std::string filename = "sip-module-script.py";
	const fs::path src = python::scripts_path() / filename;
	const fs::path backup = python::scripts_path() / (filename + ".bak");
	const fs::path dest = python::scripts_path() / (filename + ".acp");

	if (!fs::exists(backup))
	{
		const std::string utf8 = op::read_text_file(cx(), encodings::utf8, src);
		op::write_text_file(cx(), encodings::acp, dest, utf8);
		op::swap_files(cx(), src, dest, backup);
	}

	run_tool(process_runner(process()
		.binary(sip_module_exe())
		.chcp(850)
		.stdout_encoding(encodings::acp)
		.stderr_encoding(encodings::acp)
		.arg("--sip-h")
		.arg("PyQt5.zip")
		.cwd(source_path())));
}

fs::path sip::download_file()
{
	return paths::cache() / ("sip-" + version() + ".tar.gz");
}

}	// namespace
