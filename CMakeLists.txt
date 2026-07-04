cmake_minimum_required(VERSION 3.16)

project(DailyStockObserver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network)

qt_add_executable(DailyStockObserver
    main.cpp
    mainwindow.h
    mainwindow.cpp
    DatabaseManager.h
    DatabaseManager.cpp
    AddEventDialog.h
    AddEventDialog.cpp
    SettingsDialog.h
    SettingsDialog.cpp
    GoalsPanel.h
    GoalsPanel.cpp
    AddGoalDialog.h
    AddGoalDialog.cpp
    PortfolioFetcher.h
    PortfolioFetcher.cpp
    AlphaEngineFetcher.h
    AlphaEngineFetcher.cpp
    HistoricalPriceFetcher.h
    HistoricalPriceFetcher.cpp
    HistoricalPriceChartWidget.h
    HistoricalPriceChartWidget.cpp
    HistoricalPriceAnalysisDialog.h
    HistoricalPriceAnalysisDialog.cpp
    PortfolioPanel.h
    PortfolioPanel.cpp
    AddAssetDialog.h
    AddAssetDialog.cpp
    TickerSearchEdit.h
    TickerSearchEdit.cpp
    EdgarFetcher.h
    EdgarFetcher.cpp
    CompanyAnalyzer.h
    CompanyAnalyzer.cpp
    CompanyAnalysisPanel.h
    CompanyAnalysisPanel.cpp
    PeersFetcher.h
    PeersFetcher.cpp
    WorldHeatmapWidget.h
    WorldHeatmapWidget.cpp
    PeersPanel.h
    PeersPanel.cpp
    TileManager.h
    TileManager.cpp
    CountryData.h
    CountryData.cpp
    LocalDataSources.h
    LocalDataSources.cpp
    CountryBorders.h
    CountryBorders.cpp
    CountrySubdivisions.h
    CountrySubdivisions.cpp
    MapPanel.h
    MapPanel.cpp
    CountryInfoDialog.h
    CountryInfoDialog.cpp
    RegionInfoDialog.h
    RegionInfoDialog.cpp
    FinancialCalculatorPanel.h
    FinancialCalculatorPanel.cpp
    MainHubPanel.h
    MainHubPanel.cpp
    ProfileManager.h
    ProfileManager.cpp
    ProfileSelectionDialog.h
    ProfileSelectionDialog.cpp
    DialogUtils.h
    DialogUtils.cpp
    resources.qrc
)

target_link_libraries(DailyStockObserver PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)

add_custom_command(TARGET DailyStockObserver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/python
        $<TARGET_FILE_DIR:DailyStockObserver>/python
)

set_target_properties(DailyStockObserver PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# qt_finalize_executable wywoływane automatycznie przez qt_add_executable
