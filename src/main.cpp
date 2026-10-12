#include <cstdlib>

#include "keywave/app.hpp"
#include "keywave/config.hpp"

int main(int argc, char* argv[]) {
  const auto parse_result = keywave::ParseConfig(argc, argv);
  keywave::App app;
  switch (parse_result.status) {
    case keywave::ParseStatus::kHelpRequested:
    case keywave::ParseStatus::kVersionRequested:
      return EXIT_SUCCESS;
    case keywave::ParseStatus::kListDevicesRequested:
      app.PrintDeviceList();
      return EXIT_SUCCESS;
    case keywave::ParseStatus::kError:
      return EXIT_FAILURE;
    case keywave::ParseStatus::kSuccess:
      return app.Run(parse_result.config);
  }

  return EXIT_FAILURE;
}
