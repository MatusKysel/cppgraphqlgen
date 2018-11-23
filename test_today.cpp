// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Today.h"
#include "GraphQLGrammar.h"

#include <iostream>
#include <stdexcept>
#include <cstdio>

#include <tao/pegtl.hpp>

using namespace facebook::graphql;

int main(int argc, char** argv)
{
	std::vector<unsigned char> binAppointmentId;
	std::vector<unsigned char> binTaskId;
	std::vector<unsigned char> binFolderId;

	std::string fakeAppointmentId("fakeAppointmentId");
	binAppointmentId.resize(fakeAppointmentId.size());
	std::copy(fakeAppointmentId.cbegin(), fakeAppointmentId.cend(), binAppointmentId.begin());

	std::string fakeTaskId("fakeTaskId");
	binTaskId.resize(fakeTaskId.size());
	std::copy(fakeTaskId.cbegin(), fakeTaskId.cend(), binTaskId.begin());

	std::string fakeFolderId("fakeFolderId");
	binFolderId.resize(fakeFolderId.size());
	std::copy(fakeFolderId.cbegin(), fakeFolderId.cend(), binFolderId.begin());

	auto query = std::make_shared<today::Query>(
		[&binAppointmentId]() -> std::vector<std::shared_ptr<today::Appointment>>
	{
		std::cout << "Called getAppointments..." << std::endl;
		return { std::make_shared<today::Appointment>(std::move(binAppointmentId), "tomorrow", "Lunch?", false) };
	}, [&binTaskId]() -> std::vector<std::shared_ptr<today::Task>>
	{
		std::cout << "Called getTasks..." << std::endl;
		return { std::make_shared<today::Task>(std::move(binTaskId), "Don't forget", true) };
	}, [&binFolderId]() -> std::vector<std::shared_ptr<today::Folder>>
	{
		std::cout << "Called getUnreadCounts..." << std::endl;
		return { std::make_shared<today::Folder>(std::move(binFolderId), "\"Fake\" Inbox", 3) };
	});
	auto mutation = std::make_shared<today::Mutation>(
		[](today::CompleteTaskInput&& input) -> std::shared_ptr<today::CompleteTaskPayload>
	{
		return std::make_shared<today::CompleteTaskPayload>(
			std::make_shared<today::Task>(std::move(input.id), "Mutated Task!", *(input.isComplete)),
			std::move(input.clientMutationId)
		);
	});
	auto subscription = std::make_shared<today::Subscription>();
	auto service = std::make_shared<today::Operations>(query, mutation, subscription);

	std::cout << "Created the service..." << std::endl;

	try
	{
		std::string input;
		std::unique_ptr<tao::pegtl::file_input<>> file;
		std::unique_ptr<grammar::ast_node> ast;

		if (argc > 1)
		{
			file.reset(new tao::pegtl::file_input<>(argv[1]));
			ast = grammar::parseFile(std::move(*file));
		}
		else
		{
			std::string line;

			while (std::getline(std::cin, line))
			{
				input.append(line);
			}

			ast = grammar::parseString(input.c_str());
		}

		if (!ast)
		{
			std::cerr << "Unknown error!" << std::endl;
			std::cerr << std::endl;
			return 1;
		}

		std::cout << "Executing query..." << std::endl;

		utility::ostringstream_t output;
		output << service->resolve(*ast, ((argc > 2) ? argv[2] : ""), web::json::value::object().as_object());
		std::cout << utility::conversions::to_utf8string(output.str()) << std::endl;
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
