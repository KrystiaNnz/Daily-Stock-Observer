#!/usr/bin/env python3
"""Financial calculator helpers for Daily Stock Observer."""

from __future__ import annotations

import json
import random
import sys
import time


def emit(payload: dict) -> None:
    print(json.dumps(payload, ensure_ascii=False))


def parse_float(raw: str, field: str) -> float:
    try:
        return float(raw.replace(",", "."))
    except ValueError:
        raise ValueError(f"Nieprawidlowa wartosc pola {field}: {raw}")


def real_return(nominal_rate: float, inflation_rate: float) -> float:
    denominator = 1.0 + inflation_rate
    if denominator == 0:
        raise ValueError("Inflacja nie moze wynosic -100%.")
    return (1.0 + nominal_rate) / denominator - 1.0


def format_pct(rate: float) -> str:
    return f"{rate * 100:.2f}%"


def format_choice_value(value: float, unit: str) -> str:
    if unit == "%":
        return f"{value:.2f}%"
    if unit == "szt.":
        return f"{value:.0f} szt."
    if unit:
        return f"{value:.2f} {unit}"
    return f"{value:.2f}"


def build_numeric_choices(answer: float, unit: str) -> tuple[list[dict], str]:
    rounded_answer = round(answer, 2)
    scale = max(abs(rounded_answer), 1.0)
    if unit == "%":
        offsets = [1.0, -1.0, 2.0, -2.0, 0.5, -0.5]
    elif unit == "":
        offsets = [0.10, -0.10, 0.20, -0.20, 0.05, -0.05]
    elif unit == "lat":
        offsets = [0.25, -0.25, 0.50, -0.50, 1.0, -1.0]
    elif unit == "szt.":
        offsets = [10, -10, 20, -20, 50, -50]
    else:
        offsets = [scale * 0.08, -scale * 0.08, scale * 0.15, -scale * 0.15, scale * 0.25, -scale * 0.25]

    values = [rounded_answer]
    for offset in offsets:
        candidate = round(rounded_answer + offset, 2)
        if candidate != rounded_answer and candidate not in values:
            values.append(candidate)
        if len(values) == 4:
            break

    while len(values) < 4:
        candidate = round(rounded_answer + len(values) * max(scale * 0.11, 0.11), 2)
        if candidate not in values:
            values.append(candidate)

    random.shuffle(values)
    correct_index = values.index(rounded_answer)
    keys = ["A", "B", "C", "D"]
    choices = [
        {"key": keys[index], "label": format_choice_value(value, unit), "value": value}
        for index, value in enumerate(values)
    ]
    return choices, keys[correct_index]


def coupon_bond_price(face_value: float, coupon: float, ytm: float, years: int) -> float:
    return sum(coupon / ((1.0 + ytm) ** year) for year in range(1, years + 1)) + face_value / ((1.0 + ytm) ** years)


def macaulay_duration(face_value: float, coupon: float, ytm: float, years: int) -> tuple[float, float]:
    weighted_pv = 0.0
    price = 0.0
    for year in range(1, years + 1):
        cashflow = coupon + (face_value if year == years else 0.0)
        pv = cashflow / ((1.0 + ytm) ** year)
        price += pv
        weighted_pv += year * pv
    return weighted_pv / price, price


def fixing_result(buy_orders: list[tuple[float, int]], sell_orders: list[tuple[float, int]], reference_price: float) -> tuple[float, int]:
    candidates = sorted({price for price, _ in buy_orders + sell_orders})
    ranked = []
    for price in candidates:
        buy_volume = sum(qty for limit, qty in buy_orders if limit >= price)
        sell_volume = sum(qty for limit, qty in sell_orders if limit <= price)
        executable = min(buy_volume, sell_volume)
        imbalance = abs(buy_volume - sell_volume)
        ranked.append((executable, -imbalance, -abs(price - reference_price), price))
    best = max(ranked)
    return best[3], best[0]


def format_order_book(buy_orders: list[tuple[float, int]], sell_orders: list[tuple[float, int]]) -> str:
    buy_text = ", ".join(f"{qty} szt. po {price:.2f}" for price, qty in sorted(buy_orders, reverse=True))
    sell_text = ", ".join(f"{qty} szt. po {price:.2f}" for price, qty in sorted(sell_orders))
    return f"Kupno: {buy_text}. Sprzedaz: {sell_text}."


def calculate_investment(args: list[str]) -> dict:
    if len(args) != 3:
        raise ValueError("Tryb investment wymaga: zysk, naklad, inflacja_pct.")

    profit = parse_float(args[0], "zysk")
    investment = parse_float(args[1], "naklad")
    inflation_pct = parse_float(args[2], "inflacja")

    if investment <= 0:
        raise ValueError("Naklad inwestycyjny musi byc wiekszy od zera.")

    nominal = profit / investment
    inflation = inflation_pct / 100.0
    real = real_return(nominal, inflation)

    return {
        "mode": "investment",
        "nominal_return": nominal,
        "nominal_return_pct": nominal * 100.0,
        "real_return": real,
        "real_return_pct": real * 100.0,
        "formula": "r = Z / I; r_real = (1 + r_nom) / (1 + r_inf) - 1",
        "explanation": (
            f"Zysk Z = {profit:.2f}, naklad I = {investment:.2f}.\n"
            f"Nominalnie: r = {profit:.2f} / {investment:.2f} = {format_pct(nominal)}.\n"
            f"Po inflacji {inflation_pct:.2f}%: r_real = {format_pct(real)}."
        ),
    }


def calculate_stock(args: list[str]) -> dict:
    if len(args) != 4:
        raise ValueError("Tryb stock wymaga: cena_zakupu, cena_sprzedazy, dywidenda, inflacja_pct.")

    buy_price = parse_float(args[0], "cena_zakupu")
    sell_price = parse_float(args[1], "cena_sprzedazy")
    dividend = parse_float(args[2], "dywidenda")
    inflation_pct = parse_float(args[3], "inflacja")

    if buy_price <= 0:
        raise ValueError("Cena zakupu musi byc wieksza od zera.")

    profit = (sell_price - buy_price) + dividend
    nominal = profit / buy_price
    inflation = inflation_pct / 100.0
    real = real_return(nominal, inflation)

    return {
        "mode": "stock",
        "nominal_return": nominal,
        "nominal_return_pct": nominal * 100.0,
        "real_return": real,
        "real_return_pct": real * 100.0,
        "formula": "r = ((C_s - C_k) + D) / C_k; r_real = (1 + r_nom) / (1 + r_inf) - 1",
        "explanation": (
            f"C_k = {buy_price:.4f}, C_s = {sell_price:.4f}, D = {dividend:.4f}.\n"
            f"Zysk = ({sell_price:.4f} - {buy_price:.4f}) + {dividend:.4f} = {profit:.4f}.\n"
            f"Nominalnie: r = {profit:.4f} / {buy_price:.4f} = {format_pct(nominal)}.\n"
            f"Po inflacji {inflation_pct:.2f}%: r_real = {format_pct(real)}."
        ),
    }


DEPARTMENTS = {
    "financial_math_mix": {
        "topic": "Matematyka finansowa - miks zadan",
        "templates": [
            "investment_return",
            "real_return",
            "nominal_from_real_inflation",
            "stock_return",
            "effective_annual_rate",
            "future_value",
            "present_value",
            "annuity_future_value",
            "annuity_present_value",
            "perpetuity_present_value",
            "npv",
            "profitability_index",
            "single_cashflow_irr",
            "mirr",
            "equivalent_annual_annuity",
        ],
    },
    "debt_instruments_mix": {
        "topic": "Instrumenty dluzne - miks zadan",
        "templates": [
            "debt_equal_principal_interest",
            "debt_annuity_loan_payment",
            "debt_zero_coupon_price",
            "debt_zero_coupon_nominal",
            "debt_zero_coupon_ytm",
            "debt_coupon_bond_price",
            "debt_macaulay_duration",
            "debt_modified_duration_price_change",
            "debt_convexity_price_change",
        ],
    },
    "exchange_trading_techniques_mix": {
        "topic": "Techniki notowan gieldowych - miks zadan",
        "templates": [
            "tng_fixing_price",
            "tng_fixing_volume",
            "tng_continuous_buy_execution_price",
            "tng_continuous_sell_execution_price",
            "tng_tick_size_validation",
        ],
    },
    "derivatives_mix": {
        "topic": "Instrumenty pochodne - miks zadan",
        "templates": [
            "derivative_forward_long_payoff",
            "derivative_forward_short_payoff",
            "derivative_call_payoff",
            "derivative_put_payoff",
            "derivative_protective_put_payoff",
            "derivative_put_call_parity_call",
            "derivative_put_call_parity_put",
            "derivative_delta_new_option_price",
            "derivative_delta_estimation",
        ],
    },
    "portfolio_analysis_mix": {
        "topic": "Analiza portfelowa - miks zadan",
        "templates": [
            "portfolio_expected_return",
            "portfolio_two_asset_std",
            "portfolio_beta_covariance",
            "portfolio_beta_correlation",
            "portfolio_capm_expected_return",
            "portfolio_wacc",
            "portfolio_sharpe_ratio",
            "portfolio_treynor_ratio",
            "portfolio_jensen_alpha",
        ],
    },
    "ratio_accounting_mix": {
        "topic": "Analiza wskaznikowa i rachunkowosc - miks zadan",
        "templates": [
            "ratio_average_collection_period",
            "ratio_inventory_processing_period",
            "ratio_cash_conversion_cycle",
            "ratio_dupont_asset_turnover",
            "ratio_dupont_net_margin",
            "ratio_gross_profit_margin",
            "ratio_gordon_trailing_pe",
            "ratio_gordon_required_return",
            "ratio_gordon_retention_rate",
            "ratio_dol",
            "ratio_fixed_costs_from_dol",
            "ratio_dtl",
            "ratio_cost_of_debt_from_roc",
        ],
    },
    "return_rates": {
        "topic": "Stopa zwrotu i inflacja",
        "templates": [
            "investment_return",
            "stock_return",
            "real_return",
            "nominal_from_real_inflation",
            "effective_annual_rate",
        ],
    },
    "time_value_mix": {
        "topic": "Wartosc pieniadza w czasie - miks",
        "templates": [
            "future_value",
            "present_value",
            "annuity_future_value",
            "annuity_present_value",
            "perpetuity_present_value",
            "perpetuity_payment",
        ],
    },
    "investment_evaluation_mix": {
        "topic": "Ocena projektow inwestycyjnych - miks",
        "templates": [
            "npv",
            "profitability_index",
            "single_cashflow_irr",
            "mirr",
            "equivalent_annual_annuity",
            "discounted_payback",
        ],
    },
    "investment_return": {
        "topic": "Stopa zwrotu z inwestycji",
        "templates": ["investment_return"],
    },
    "stock_return": {
        "topic": "Stopa zwrotu z akcji",
        "templates": ["stock_return"],
    },
    "real_return": {
        "topic": "Realna stopa zwrotu",
        "templates": ["real_return"],
    },
    "nominal_from_real_inflation": {
        "topic": "Nominalna stopa zwrotu",
        "templates": ["nominal_from_real_inflation"],
    },
    "effective_annual_rate": {
        "topic": "Efektywna stopa roczna",
        "templates": ["effective_annual_rate"],
    },
    "future_value": {
        "topic": "Wartosc przyszla",
        "templates": ["future_value"],
    },
    "present_value": {
        "topic": "Wartosc biezaca",
        "templates": ["present_value"],
    },
    "annuity_future_value": {
        "topic": "Wartosc przyszla renty",
        "templates": ["annuity_future_value"],
    },
    "annuity_present_value": {
        "topic": "Wartosc biezaca renty",
        "templates": ["annuity_present_value"],
    },
    "perpetuity_present_value": {
        "topic": "Wartosc biezaca renty wieczystej",
        "templates": ["perpetuity_present_value"],
    },
    "perpetuity_payment": {
        "topic": "Platnosc renty wieczystej",
        "templates": ["perpetuity_payment"],
    },
    "npv": {
        "topic": "NPV",
        "templates": ["npv"],
    },
    "profitability_index": {
        "topic": "PI",
        "templates": ["profitability_index"],
    },
    "npv_pi": {
        "topic": "NPV i PI",
        "templates": ["npv", "profitability_index"],
    },
    "single_cashflow_irr": {
        "topic": "IRR",
        "templates": ["single_cashflow_irr"],
    },
    "mirr": {
        "topic": "MIRR",
        "templates": ["mirr"],
    },
    "irr_mirr": {
        "topic": "IRR i MIRR",
        "templates": ["single_cashflow_irr", "mirr"],
    },
    "equivalent_annual_annuity": {
        "topic": "EAA",
        "templates": ["equivalent_annual_annuity"],
    },
    "discounted_payback": {
        "topic": "Zdyskontowany okres zwrotu",
        "templates": ["discounted_payback"],
    },
    "project_methods": {
        "topic": "Metody oceny projektow inwestycyjnych",
        "templates": ["equivalent_annual_annuity", "discounted_payback"],
    },
    "debt_equal_principal_interest": {
        "topic": "Kredyt - rowne raty kapitalowe",
        "templates": ["debt_equal_principal_interest"],
    },
    "debt_annuity_loan_payment": {
        "topic": "Kredyt - rowne platnosci",
        "templates": ["debt_annuity_loan_payment"],
    },
    "debt_zero_coupon_price": {
        "topic": "Obligacje zerokuponowe - cena",
        "templates": ["debt_zero_coupon_price"],
    },
    "debt_zero_coupon_nominal": {
        "topic": "Obligacje zerokuponowe - nominal",
        "templates": ["debt_zero_coupon_nominal"],
    },
    "debt_zero_coupon_ytm": {
        "topic": "Obligacje zerokuponowe - YTM",
        "templates": ["debt_zero_coupon_ytm"],
    },
    "debt_coupon_bond_price": {
        "topic": "Obligacje kuponowe - cena",
        "templates": ["debt_coupon_bond_price"],
    },
    "debt_macaulay_duration": {
        "topic": "Duration Macaulaya",
        "templates": ["debt_macaulay_duration"],
    },
    "debt_modified_duration_price_change": {
        "topic": "Modified duration - zmiana ceny",
        "templates": ["debt_modified_duration_price_change"],
    },
    "debt_convexity_price_change": {
        "topic": "Convexity - zmiana ceny",
        "templates": ["debt_convexity_price_change"],
    },
    "tng_fixing_price": {
        "topic": "TNG - kurs fixingowy",
        "templates": ["tng_fixing_price"],
    },
    "tng_fixing_volume": {
        "topic": "TNG - teoretyczny wolumen",
        "templates": ["tng_fixing_volume"],
    },
    "tng_continuous_buy_execution_price": {
        "topic": "TNG - realizacja zlecenia kupna",
        "templates": ["tng_continuous_buy_execution_price"],
    },
    "tng_continuous_sell_execution_price": {
        "topic": "TNG - realizacja zlecenia sprzedazy",
        "templates": ["tng_continuous_sell_execution_price"],
    },
    "tng_tick_size_validation": {
        "topic": "TNG - krok notowan",
        "templates": ["tng_tick_size_validation"],
    },
    "derivative_forward_long_payoff": {
        "topic": "Pochodne - long forward",
        "templates": ["derivative_forward_long_payoff"],
    },
    "derivative_forward_short_payoff": {
        "topic": "Pochodne - short forward",
        "templates": ["derivative_forward_short_payoff"],
    },
    "derivative_call_payoff": {
        "topic": "Pochodne - payoff call",
        "templates": ["derivative_call_payoff"],
    },
    "derivative_put_payoff": {
        "topic": "Pochodne - payoff put",
        "templates": ["derivative_put_payoff"],
    },
    "derivative_protective_put_payoff": {
        "topic": "Pochodne - protective put",
        "templates": ["derivative_protective_put_payoff"],
    },
    "derivative_put_call_parity_call": {
        "topic": "Pochodne - parytet put-call: call",
        "templates": ["derivative_put_call_parity_call"],
    },
    "derivative_put_call_parity_put": {
        "topic": "Pochodne - parytet put-call: put",
        "templates": ["derivative_put_call_parity_put"],
    },
    "derivative_delta_new_option_price": {
        "topic": "Pochodne - delta i cena opcji",
        "templates": ["derivative_delta_new_option_price"],
    },
    "derivative_delta_estimation": {
        "topic": "Pochodne - estymacja delty",
        "templates": ["derivative_delta_estimation"],
    },
    "portfolio_expected_return": {
        "topic": "Analiza portfela - stopa zwrotu",
        "templates": ["portfolio_expected_return"],
    },
    "portfolio_two_asset_std": {
        "topic": "Analiza portfela - ryzyko 2 aktywow",
        "templates": ["portfolio_two_asset_std"],
    },
    "portfolio_beta_covariance": {
        "topic": "Analiza portfela - beta z kowariancji",
        "templates": ["portfolio_beta_covariance"],
    },
    "portfolio_beta_correlation": {
        "topic": "Analiza portfela - beta z korelacji",
        "templates": ["portfolio_beta_correlation"],
    },
    "portfolio_capm_expected_return": {
        "topic": "Analiza portfela - CAPM",
        "templates": ["portfolio_capm_expected_return"],
    },
    "portfolio_wacc": {
        "topic": "Analiza portfela - WACC",
        "templates": ["portfolio_wacc"],
    },
    "portfolio_sharpe_ratio": {
        "topic": "Analiza portfela - Sharpe",
        "templates": ["portfolio_sharpe_ratio"],
    },
    "portfolio_treynor_ratio": {
        "topic": "Analiza portfela - Treynor",
        "templates": ["portfolio_treynor_ratio"],
    },
    "portfolio_jensen_alpha": {
        "topic": "Analiza portfela - Jensen alpha",
        "templates": ["portfolio_jensen_alpha"],
    },
    "ratio_average_collection_period": {
        "topic": "Wskazniki aktywnosci - sredni okres inkasa",
        "templates": ["ratio_average_collection_period"],
    },
    "ratio_inventory_processing_period": {
        "topic": "Wskazniki aktywnosci - okres obrotu zapasami",
        "templates": ["ratio_inventory_processing_period"],
    },
    "ratio_cash_conversion_cycle": {
        "topic": "Wskazniki aktywnosci - cykl konwersji gotowki",
        "templates": ["ratio_cash_conversion_cycle"],
    },
    "ratio_dupont_asset_turnover": {
        "topic": "Wskazniki rentownosci - DuPont: obrot aktywami",
        "templates": ["ratio_dupont_asset_turnover"],
    },
    "ratio_dupont_net_margin": {
        "topic": "Wskazniki rentownosci - DuPont: marza netto",
        "templates": ["ratio_dupont_net_margin"],
    },
    "ratio_gross_profit_margin": {
        "topic": "Wskazniki rentownosci - marza brutto",
        "templates": ["ratio_gross_profit_margin"],
    },
    "ratio_gordon_trailing_pe": {
        "topic": "Model Gordona - trailing P/E",
        "templates": ["ratio_gordon_trailing_pe"],
    },
    "ratio_gordon_required_return": {
        "topic": "Model Gordona - wymagana stopa zwrotu",
        "templates": ["ratio_gordon_required_return"],
    },
    "ratio_gordon_retention_rate": {
        "topic": "Model Gordona - stopa zyskow zatrzymanych",
        "templates": ["ratio_gordon_retention_rate"],
    },
    "ratio_dol": {
        "topic": "Dzwignie - DOL",
        "templates": ["ratio_dol"],
    },
    "ratio_fixed_costs_from_dol": {
        "topic": "Dzwignie - koszty stale z DOL",
        "templates": ["ratio_fixed_costs_from_dol"],
    },
    "ratio_dtl": {
        "topic": "Dzwignie - DTL",
        "templates": ["ratio_dtl"],
    },
    "ratio_cost_of_debt_from_roc": {
        "topic": "Wskazniki rentownosci - koszt dlugu z ROC",
        "templates": ["ratio_cost_of_debt_from_roc"],
    },
}

DEPARTMENT_ALIASES = {
    "financial_math": "financial_math_mix",
}


def resolve_department(department: str) -> str:
    key = department or "financial_math_mix"
    key = DEPARTMENT_ALIASES.get(key, key)
    if key not in DEPARTMENTS:
        raise ValueError(f"Nieznany dzial zadan: {department}")
    return key


def generate_financial_math_exercise(department: str = "financial_math_mix") -> dict:
    department = resolve_department(department)
    topic = DEPARTMENTS[department]["topic"]
    templates = DEPARTMENTS[department]["templates"]
    template = random.choice(templates)

    if template == "investment_return":
        investment = random.choice([1000, 1500, 2000, 2500, 5000, 10000])
        profit_rate = random.choice([0.06, 0.08, 0.10, 0.12, 0.15, 0.20])
        profit = round(investment * profit_rate, 2)
        answer = profit / investment * 100.0
        question = (
            f"Inwestor poniosl naklad I = {investment:.2f} PLN i osiagnal zysk "
            f"Z = {profit:.2f} PLN. Oblicz nominalna stope zwrotu r = Z / I. "
            "Podaj wynik w procentach."
        )
        explanation = f"r = {profit:.2f} / {investment:.2f} = {answer:.2f}%."
        xp_reward = 5
        answer_unit = "%"
        tolerance = 0.05
    elif template == "stock_return":
        buy = random.choice([40, 50, 80, 100, 120, 150])
        sell = buy + random.choice([-8, -5, 4, 8, 12, 20])
        dividend = random.choice([0, 1.5, 2, 3, 4, 5])
        answer = ((sell - buy) + dividend) / buy * 100.0
        question = (
            f"Akcje kupiono po C_k = {buy:.2f} PLN, sprzedano po C_s = {sell:.2f} PLN, "
            f"a dywidenda D wyniosla {dividend:.2f} PLN na akcje. Oblicz stope zwrotu "
            "r = ((C_s - C_k) + D) / C_k. Podaj wynik w procentach."
        )
        explanation = (
            f"r = (({sell:.2f} - {buy:.2f}) + {dividend:.2f}) / {buy:.2f} "
            f"= {answer:.2f}%."
        )
        xp_reward = 10
        answer_unit = "%"
        tolerance = 0.05
    elif template == "real_return":
        nominal_pct = random.choice([6, 8, 10, 12, 15, 18, 20])
        inflation_pct = random.choice([2, 3, 4, 5, 6, 8])
        nominal = nominal_pct / 100.0
        inflation = inflation_pct / 100.0
        answer = real_return(nominal, inflation) * 100.0
        question = (
            f"Nominalna stopa zwrotu wynosi r_nom = {nominal_pct:.2f}%, "
            f"a inflacja r_inf = {inflation_pct:.2f}%. Oblicz realna stope zwrotu "
            "r_real = (1 + r_nom) / (1 + r_inf) - 1. Podaj wynik w procentach."
        )
        explanation = (
            f"r_real = (1 + {nominal:.4f}) / (1 + {inflation:.4f}) - 1 "
            f"= {answer:.2f}%."
        )
        xp_reward = 5
        answer_unit = "%"
        tolerance = 0.05
    elif template == "nominal_from_real_inflation":
        real_pct = random.choice([4, 6, 8, 10, 12, 14])
        inflation_pct = random.choice([2, 3, 5, 7, 9])
        real = real_pct / 100.0
        inflation = inflation_pct / 100.0
        answer = ((1.0 + real) * (1.0 + inflation) - 1.0) * 100.0
        question = (
            f"Realna stopa zwrotu wynosi r_real = {real_pct:.2f}%, "
            f"a inflacja r_inf = {inflation_pct:.2f}%. Oblicz nominalna stope zwrotu "
            "r_nom = (1 + r_real) * (1 + r_inf) - 1. Podaj wynik w procentach."
        )
        explanation = (
            f"r_nom = (1 + {real:.4f}) * (1 + {inflation:.4f}) - 1 "
            f"= {answer:.2f}%."
        )
        xp_reward = 5
        answer_unit = "%"
        tolerance = 0.05
    elif template == "effective_annual_rate":
        nominal_pct = random.choice([6, 8, 10, 12, 14])
        periods = random.choice([2, 4, 6, 12])
        nominal = nominal_pct / 100.0
        answer = ((1.0 + nominal / periods) ** periods - 1.0) * 100.0
        question = (
            f"Nominalna roczna stopa procentowa wynosi {nominal_pct:.2f}%, "
            f"a kapitalizacja odsetek nastepuje {periods} razy w roku. "
            "Oblicz efektywna roczna stope procentowa. Podaj wynik w procentach."
        )
        explanation = (
            f"EAR = (1 + {nominal:.4f}/{periods})^{periods} - 1 = {answer:.2f}%."
        )
        xp_reward = 10
        answer_unit = "%"
        tolerance = 0.05
    elif template == "future_value":
        principal = random.choice([1000, 2500, 5000, 7500, 10000])
        rate_pct = random.choice([4, 5, 6, 7, 8, 10])
        years = random.choice([2, 3, 4, 5, 8, 10])
        rate = rate_pct / 100.0
        answer = principal * (1.0 + rate) ** years
        question = (
            f"Wplacasz dzis {principal:.2f} PLN na {years} lat przy rocznej kapitalizacji "
            f"i stopie {rate_pct:.2f}%. Oblicz wartosc przyszla FV. Podaj wynik w PLN."
        )
        explanation = f"FV = {principal:.2f} * (1 + {rate:.4f})^{years} = {answer:.2f} PLN."
        xp_reward = 10
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "present_value":
        future_value = random.choice([5000, 8000, 12000, 15000, 20000])
        rate_pct = random.choice([4, 5, 6, 8, 10, 12])
        years = random.choice([2, 3, 4, 5, 8])
        rate = rate_pct / 100.0
        answer = future_value / ((1.0 + rate) ** years)
        question = (
            f"Za {years} lat otrzymasz {future_value:.2f} PLN. Roczna stopa dyskontowa wynosi "
            f"{rate_pct:.2f}%. Oblicz wartosc biezaca PV. Podaj wynik w PLN."
        )
        explanation = f"PV = {future_value:.2f} / (1 + {rate:.4f})^{years} = {answer:.2f} PLN."
        xp_reward = 10
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "annuity_future_value":
        payment = random.choice([500, 1000, 1500, 2000])
        rate_pct = random.choice([4, 5, 6, 8])
        years = random.choice([3, 4, 5, 6])
        rate = rate_pct / 100.0
        answer = payment * (((1.0 + rate) ** years - 1.0) / rate)
        question = (
            f"Na koniec kazdego roku wplacasz {payment:.2f} PLN przez {years} lat. "
            f"Stopa procentowa wynosi {rate_pct:.2f}%. Oblicz wartosc przyszla renty zwyklej. "
            "Podaj wynik w PLN."
        )
        explanation = (
            f"FVA = {payment:.2f} * [((1 + {rate:.4f})^{years} - 1) / {rate:.4f}] "
            f"= {answer:.2f} PLN."
        )
        xp_reward = 20
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "annuity_present_value":
        payment = random.choice([800, 1000, 1500, 2500])
        rate_pct = random.choice([5, 6, 8, 10])
        years = random.choice([3, 4, 5, 7])
        rate = rate_pct / 100.0
        answer = payment * ((1.0 - (1.0 + rate) ** (-years)) / rate)
        question = (
            f"Otrzymasz {payment:.2f} PLN na koniec kazdego roku przez {years} lat. "
            f"Stopa dyskontowa wynosi {rate_pct:.2f}%. Oblicz wartosc biezaca renty zwyklej. "
            "Podaj wynik w PLN."
        )
        explanation = (
            f"PVA = {payment:.2f} * [(1 - (1 + {rate:.4f})^-{years}) / {rate:.4f}] "
            f"= {answer:.2f} PLN."
        )
        xp_reward = 20
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "perpetuity_present_value":
        payment = random.choice([400, 600, 800, 1000, 1200, 1500])
        rate_pct = random.choice([4, 5, 6, 8, 10, 12])
        rate = rate_pct / 100.0
        answer = payment / rate
        question = (
            f"Renta wieczysta wyplaca {payment:.2f} PLN na koniec kazdego roku. "
            f"Stopa dyskontowa wynosi {rate_pct:.2f}%. Oblicz wartosc biezaca renty "
            "wieczystej. Podaj wynik w PLN."
        )
        explanation = f"PV = PMT / r = {payment:.2f} / {rate:.4f} = {answer:.2f} PLN."
        xp_reward = 20
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "perpetuity_payment":
        present_value = random.choice([10000, 15000, 20000, 30000, 40000])
        rate_pct = random.choice([4, 5, 6, 8, 10])
        rate = rate_pct / 100.0
        answer = present_value * rate
        question = (
            f"Wartosc biezaca renty wieczystej wynosi {present_value:.2f} PLN, "
            f"a stopa dyskontowa {rate_pct:.2f}%. Oblicz roczna platnosc PMT. "
            "Podaj wynik w PLN."
        )
        explanation = f"PMT = PV * r = {present_value:.2f} * {rate:.4f} = {answer:.2f} PLN."
        xp_reward = 20
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "npv":
        outlay = random.choice([10000, 15000, 20000, 30000])
        cashflow = random.choice([3500, 5000, 6500, 8000])
        years = random.choice([4, 5, 6])
        rate_pct = random.choice([8, 10, 12])
        rate = rate_pct / 100.0
        pv_inflows = cashflow * ((1.0 - (1.0 + rate) ** (-years)) / rate)
        answer = pv_inflows - outlay
        question = (
            f"Projekt wymaga nakladu {outlay:.2f} PLN i daje {cashflow:.2f} PLN "
            f"na koniec kazdego roku przez {years} lat. Stopa dyskontowa wynosi {rate_pct:.2f}%. "
            "Oblicz NPV. Podaj wynik w PLN."
        )
        explanation = (
            f"PV wplywow = {cashflow:.2f} * [(1 - (1 + {rate:.4f})^-{years}) / {rate:.4f}] "
            f"= {pv_inflows:.2f}; NPV = {pv_inflows:.2f} - {outlay:.2f} = {answer:.2f} PLN."
        )
        xp_reward = 25
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "profitability_index":
        outlay = random.choice([10000, 12500, 16000, 20000])
        pv_inflows = outlay * random.choice([1.15, 1.25, 1.40, 1.60])
        answer = pv_inflows / outlay
        question = (
            f"Suma wartosci biezacych dodatnich przeplywow wynosi {pv_inflows:.2f} PLN, "
            f"a naklad inwestycyjny {outlay:.2f} PLN. Oblicz indeks zyskownosci PI. "
            "Podaj wynik jako liczbe, np. 1,25."
        )
        explanation = f"PI = {pv_inflows:.2f} / {outlay:.2f} = {answer:.2f}."
        xp_reward = 25
        answer_unit = ""
        tolerance = 0.01
    elif template == "single_cashflow_irr":
        outlay = random.choice([5000, 8000, 10000, 15000])
        years = random.choice([3, 4, 5, 6])
        irr_pct = random.choice([6, 8, 10, 12, 15])
        future_cashflow = outlay * ((1.0 + irr_pct / 100.0) ** years)
        answer = irr_pct
        question = (
            f"Inwestujesz dzis {outlay:.2f} PLN i otrzymasz jedyny przeplyw "
            f"{future_cashflow:.2f} PLN za {years} lat. Oblicz IRR. Podaj wynik w procentach."
        )
        explanation = (
            f"IRR = ({future_cashflow:.2f} / {outlay:.2f})^(1/{years}) - 1 = {answer:.2f}%."
        )
        xp_reward = 35
        answer_unit = "%"
        tolerance = 0.05
    elif template == "mirr":
        outlay = random.choice([10000, 15000, 20000, 25000])
        cashflow = random.choice([3000, 4500, 6000, 7500])
        years = random.choice([4, 5, 6])
        finance_pct = random.choice([6, 8, 10])
        reinvest_pct = random.choice([5, 7, 9])
        reinvest = reinvest_pct / 100.0
        terminal_value = sum(cashflow * ((1.0 + reinvest) ** (years - year)) for year in range(1, years + 1))
        answer = ((terminal_value / outlay) ** (1.0 / years) - 1.0) * 100.0
        question = (
            f"Projekt wymaga nakladu {outlay:.2f} PLN w t=0 i daje {cashflow:.2f} PLN "
            f"na koniec kazdego roku przez {years} lat. Stopa finansowania wynosi "
            f"{finance_pct:.2f}%, a stopa reinwestycji {reinvest_pct:.2f}%. "
            "Przy jednym nakladzie poczatkowym oblicz MIRR. Podaj wynik w procentach."
        )
        explanation = (
            f"TV dodatnich przeplywow przy {reinvest_pct:.2f}% = {terminal_value:.2f} PLN; "
            f"MIRR = ({terminal_value:.2f} / {outlay:.2f})^(1/{years}) - 1 = {answer:.2f}%."
        )
        xp_reward = 35
        answer_unit = "%"
        tolerance = 0.05
    elif template == "equivalent_annual_annuity":
        npv_value = random.choice([5000, 8000, 12000, 15000, 20000])
        rate_pct = random.choice([6, 8, 10, 12])
        years = random.choice([3, 4, 5, 6])
        rate = rate_pct / 100.0
        annuity_factor = (1.0 - (1.0 + rate) ** (-years)) / rate
        answer = npv_value / annuity_factor
        question = (
            f"Projekt ma NPV = {npv_value:.2f} PLN, czas zycia {years} lat i stope "
            f"dyskontowa {rate_pct:.2f}%. Oblicz rownowazna roczna annuite EAA. "
            "Podaj wynik w PLN."
        )
        explanation = (
            f"EAA = NPV / PVIFA = {npv_value:.2f} / {annuity_factor:.4f} "
            f"= {answer:.2f} PLN."
        )
        xp_reward = 25
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "debt_equal_principal_interest":
        principal = random.choice([120000, 180000, 240000, 300000, 420000])
        years = random.choice([3, 4, 5, 8, 10])
        periods_per_year = random.choice([4, 12])
        total_periods = years * periods_per_year
        annual_rate_pct = random.choice([8, 10, 12, 14, 16])
        installment_no = random.randint(2, total_periods)
        capital_part = principal / total_periods
        balance_before = principal - capital_part * (installment_no - 1)
        periodic_rate = annual_rate_pct / 100.0 / periods_per_year
        answer = balance_before * periodic_rate
        period_name = "miesiecznych" if periods_per_year == 12 else "kwartalnych"
        question = (
            f"Kredyt {principal:.2f} PLN jest splacany metoda rownych rat kapitalowych przez {years} lat "
            f"w ratach {period_name}. Roczna stopa procentowa wynosi {annual_rate_pct:.2f}%. "
            f"Oblicz czesc odsetkowa raty nr {installment_no}. Podaj wynik w PLN."
        )
        explanation = (
            f"Czesc kapitalowa = {principal:.2f}/{total_periods} = {capital_part:.2f} PLN. "
            f"Saldo przed rata {installment_no} = {balance_before:.2f} PLN; "
            f"odsetki = {balance_before:.2f} * {periodic_rate:.4f} = {answer:.2f} PLN."
        )
        xp_reward = 15
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "debt_annuity_loan_payment":
        principal = random.choice([25000, 50000, 75000, 100000, 150000])
        years = random.choice([2, 3, 4, 5, 6])
        periods_per_year = random.choice([2, 4, 12])
        total_periods = years * periods_per_year
        annual_rate_pct = random.choice([6, 8, 10, 12, 15, 18])
        periodic_rate = annual_rate_pct / 100.0 / periods_per_year
        answer = principal * periodic_rate / (1.0 - (1.0 + periodic_rate) ** (-total_periods))
        period_name = "miesiecznych" if periods_per_year == 12 else ("kwartalnych" if periods_per_year == 4 else "polrocznych")
        question = (
            f"Kredyt {principal:.2f} PLN ma byc splacony przez {years} lat w rownych ratach {period_name}. "
            f"Roczna stopa nominalna wynosi {annual_rate_pct:.2f}%. Oblicz wysokosc raty. Podaj wynik w PLN."
        )
        explanation = (
            f"r okresowe = {periodic_rate:.4f}, n = {total_periods}. "
            f"Rata = PV*r/[1-(1+r)^-n] = {answer:.2f} PLN."
        )
        xp_reward = 20
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "debt_zero_coupon_price":
        face_value = random.choice([1000, 2000, 5000, 10000, 20000])
        years = random.choice([3, 5, 7, 10, 12])
        ytm_pct = random.choice([5, 6, 8, 10, 12, 14])
        ytm = ytm_pct / 100.0
        answer = face_value / ((1.0 + ytm) ** years)
        question = (
            f"Obligacja zerokuponowa ma nominal {face_value:.2f} PLN i termin wykupu za {years} lat. "
            f"YTM wynosi {ytm_pct:.2f}%. Oblicz jej cene. Podaj wynik w PLN."
        )
        explanation = f"P = N/(1+y)^n = {face_value:.2f}/(1+{ytm:.4f})^{years} = {answer:.2f} PLN."
        xp_reward = 10
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "debt_zero_coupon_nominal":
        price = random.choice([500, 750, 1000, 1500, 2500, 4000])
        years = random.choice([4, 6, 8, 10, 12])
        ytm_pct = random.choice([6, 8, 10, 12, 14])
        ytm = ytm_pct / 100.0
        answer = price * ((1.0 + ytm) ** years)
        question = (
            f"Cena obligacji zerokuponowej wynosi {price:.2f} PLN, do wykupu pozostalo {years} lat, "
            f"a YTM wynosi {ytm_pct:.2f}%. Oblicz nominal obligacji. Podaj wynik w PLN."
        )
        explanation = f"N = P*(1+y)^n = {price:.2f}*(1+{ytm:.4f})^{years} = {answer:.2f} PLN."
        xp_reward = 10
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "debt_zero_coupon_ytm":
        face_value = random.choice([1000, 2000, 5000, 10000])
        years = random.choice([3, 5, 7, 9, 12])
        ytm_pct = random.choice([5, 7, 9, 11, 13, 15])
        price = face_value / ((1.0 + ytm_pct / 100.0) ** years)
        answer = ((face_value / price) ** (1.0 / years) - 1.0) * 100.0
        question = (
            f"Obligacja zerokuponowa o nominale {face_value:.2f} PLN kosztuje {price:.2f} PLN, "
            f"a do wykupu pozostalo {years} lat. Oblicz YTM. Podaj wynik w procentach."
        )
        explanation = f"YTM = (N/P)^(1/n)-1 = ({face_value:.2f}/{price:.2f})^(1/{years})-1 = {answer:.2f}%."
        xp_reward = 15
        answer_unit = "%"
        tolerance = 0.05
    elif template == "debt_coupon_bond_price":
        face_value = random.choice([1000, 2000, 5000])
        coupon_rate_pct = random.choice([4, 5, 6, 8, 10])
        ytm_pct = random.choice([5, 7, 9, 11])
        years = random.choice([3, 4, 5, 6, 8])
        coupon = face_value * coupon_rate_pct / 100.0
        ytm = ytm_pct / 100.0
        answer = coupon_bond_price(face_value, coupon, ytm, years)
        question = (
            f"Obligacja kuponowa ma nominal {face_value:.2f} PLN, kupon roczny {coupon_rate_pct:.2f}% "
            f"i termin wykupu za {years} lat. YTM wynosi {ytm_pct:.2f}%. Oblicz cene obligacji. "
            "Podaj wynik w PLN."
        )
        explanation = (
            f"Kupon = {coupon:.2f} PLN. Cena = suma PV kuponow + PV nominalu = {answer:.2f} PLN."
        )
        xp_reward = 20
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "debt_macaulay_duration":
        face_value = random.choice([1000, 2000, 5000])
        coupon_rate_pct = random.choice([4, 6, 8, 10])
        ytm_pct = random.choice([5, 7, 9])
        years = random.choice([3, 4, 5, 6])
        coupon = face_value * coupon_rate_pct / 100.0
        ytm = ytm_pct / 100.0
        answer, price = macaulay_duration(face_value, coupon, ytm, years)
        question = (
            f"Obligacja kuponowa ma nominal {face_value:.2f} PLN, kupon roczny {coupon_rate_pct:.2f}%, "
            f"YTM {ytm_pct:.2f}% i termin wykupu za {years} lat. Oblicz duration Macaulaya. "
            "Podaj wynik w latach."
        )
        explanation = (
            f"Cena obligacji = {price:.2f} PLN. Duration Macaulaya = suma(t*PV(CF_t))/P = {answer:.2f} lat."
        )
        xp_reward = 25
        answer_unit = "lat"
        tolerance = 0.05
    elif template == "debt_modified_duration_price_change":
        modified_duration = random.choice([3.8, 5.2, 7.4, 9.1, 12.5])
        old_yield_pct = random.choice([5.50, 6.25, 7.00, 8.40])
        delta_bp = random.choice([-75, -50, -25, 25, 50, 75])
        new_yield_pct = old_yield_pct + delta_bp / 100.0
        delta_y = (new_yield_pct - old_yield_pct) / 100.0
        answer = -modified_duration * delta_y * 100.0
        question = (
            f"Zmodyfikowany czas trwania obligacji wynosi {modified_duration:.2f}. "
            f"YTM zmienia sie z {old_yield_pct:.2f}% do {new_yield_pct:.2f}%. "
            "Oszacuj procentowa zmiane ceny obligacji metoda modified duration. Podaj wynik w procentach."
        )
        explanation = f"Delta P/P ~= -D_mod * Delta y = -{modified_duration:.2f} * {delta_y:.4f} = {answer:.2f}%."
        xp_reward = 25
        answer_unit = "%"
        tolerance = 0.05
    elif template == "debt_convexity_price_change":
        modified_duration = random.choice([4.5, 6.8, 8.2, 10.5])
        convexity = random.choice([32.0, 55.0, 80.0, 120.0])
        old_yield_pct = random.choice([5.0, 6.0, 7.5, 9.0])
        delta_bp = random.choice([-100, -75, -50, 50, 75, 100])
        new_yield_pct = old_yield_pct + delta_bp / 100.0
        delta_y = (new_yield_pct - old_yield_pct) / 100.0
        answer = (-modified_duration * delta_y + 0.5 * convexity * (delta_y ** 2)) * 100.0
        question = (
            f"Obligacja ma modified duration {modified_duration:.2f} i convexity {convexity:.2f}. "
            f"YTM zmienia sie z {old_yield_pct:.2f}% do {new_yield_pct:.2f}%. "
            "Oszacuj procentowa zmiane ceny z uwzglednieniem wypuklosci. Podaj wynik w procentach."
        )
        explanation = (
            f"Delta P/P ~= -D_mod*Delta y + 0.5*C*(Delta y)^2 = {answer:.2f}%."
        )
        xp_reward = 35
        answer_unit = "%"
        tolerance = 0.05
    elif template == "tng_fixing_price":
        reference_price = random.choice([24.50, 37.20, 58.80, 91.40])
        base = reference_price
        levels = [round(base - 0.20, 2), round(base - 0.10, 2), round(base, 2), round(base + 0.10, 2), round(base + 0.20, 2)]
        buy_orders = [(levels[3], random.choice([80, 100, 120])), (levels[2], random.choice([120, 150, 180])), (levels[1], random.choice([60, 90, 110]))]
        sell_orders = [(levels[1], random.choice([70, 90, 110])), (levels[2], random.choice([100, 130, 160])), (levels[3], random.choice([80, 100, 120]))]
        answer, volume = fixing_result(buy_orders, sell_orders, reference_price)
        question = (
            "W fazie przed otwarciem arkusz zlecen wyglada nastepujaco. "
            f"Kurs odniesienia wynosi {reference_price:.2f}. {format_order_book(buy_orders, sell_orders)} "
            "Wyznacz kurs fixingowy zgodnie z zasada maksymalizacji wolumenu, minimalizacji nierownowagi "
            "i bliskosci kursu odniesienia. Podaj kurs."
        )
        explanation = f"Dla najlepszego kursu realizowany wolumen wynosi {volume} szt.; kurs fixingowy = {answer:.2f}."
        xp_reward = 25
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "tng_fixing_volume":
        reference_price = random.choice([18.00, 42.50, 76.00])
        levels = [round(reference_price - 0.20, 2), round(reference_price - 0.10, 2), round(reference_price, 2), round(reference_price + 0.10, 2)]
        buy_orders = [(levels[3], random.choice([100, 120])), (levels[2], random.choice([80, 100])), (levels[1], random.choice([60, 90]))]
        sell_orders = [(levels[1], random.choice([70, 90])), (levels[2], random.choice([100, 120])), (levels[3], random.choice([50, 80]))]
        price, answer = fixing_result(buy_orders, sell_orders, reference_price)
        question = (
            "W fazie przed zamknieciem wyznaczany jest kurs jednolity. "
            f"Kurs odniesienia: {reference_price:.2f}. {format_order_book(buy_orders, sell_orders)} "
            "Oblicz teoretyczny wolumen realizacji dla kursu fixingowego. Podaj liczbe sztuk."
        )
        explanation = f"Kurs fixingowy = {price:.2f}; wolumen realizacji = min(skumulowane kupno, skumulowana sprzedaz) = {answer} szt."
        xp_reward = 20
        answer_unit = "szt."
        tolerance = 0.01
    elif template == "tng_continuous_buy_execution_price":
        best_ask = random.choice([12.40, 25.10, 48.75, 101.20])
        ask_qty = random.choice([100, 150, 200, 300])
        buy_qty = random.choice([20, 50, 80, 100])
        buy_limit = round(best_ask + random.choice([0.10, 0.20, 0.50]), 2)
        answer = best_ask
        question = (
            f"W fazie notowan ciaglych najlepsza oferta sprzedazy to {ask_qty} szt. po {best_ask:.2f}. "
            f"Do arkusza trafia zlecenie kupna {buy_qty} szt. z limitem {buy_limit:.2f}. "
            "Po jakim kursie zostanie zrealizowana transakcja? Podaj kurs."
        )
        explanation = f"Limit kupna {buy_limit:.2f} jest wyzszy od najlepszej sprzedazy, wiec zlecenie realizuje sie po {answer:.2f}."
        xp_reward = 15
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "tng_continuous_sell_execution_price":
        best_bid = random.choice([8.80, 31.50, 64.25, 120.00])
        bid_qty = random.choice([100, 150, 250, 400])
        sell_qty = random.choice([20, 50, 90, 100])
        sell_limit = round(best_bid - random.choice([0.10, 0.20, 0.50]), 2)
        answer = best_bid
        question = (
            f"W fazie notowan ciaglych najlepsza oferta kupna to {bid_qty} szt. po {best_bid:.2f}. "
            f"Do arkusza trafia zlecenie sprzedazy {sell_qty} szt. z limitem {sell_limit:.2f}. "
            "Po jakim kursie zostanie zrealizowana transakcja? Podaj kurs."
        )
        explanation = f"Limit sprzedazy {sell_limit:.2f} jest nizszy od najlepszej oferty kupna, wiec transakcja zachodzi po {answer:.2f}."
        xp_reward = 15
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "tng_tick_size_validation":
        tick = random.choice([0.01, 0.05, 0.10, 0.50])
        base = random.choice([10.00, 25.00, 50.00, 100.00])
        valid_prices = [round(base + tick * n, 2) for n in random.sample(range(1, 10), 2)]
        invalid_prices = [round(base + tick * random.choice([1.5, 2.5, 3.5]), 2), round(base + tick * random.choice([4.2, 5.4, 6.6]), 2)]
        prices = valid_prices + invalid_prices
        random.shuffle(prices)
        answer = sum(1 for price in prices if abs(round((price - base) / tick) * tick - (price - base)) < 0.000001)
        question = (
            f"Minimalna wielkosc zmiany ceny wynosi {tick:.2f}. "
            f"Sprawdz limity: {', '.join(f'{price:.2f}' for price in prices)}. "
            "Ile z podanych limitow jest zgodnych z krokiem notowan?"
        )
        explanation = f"Zgodne z krokiem {tick:.2f} sa {int(answer)} limity z podanej listy."
        xp_reward = 10
        answer_unit = ""
        tolerance = 0.01
    elif template == "derivative_forward_long_payoff":
        forward_price = random.choice([80, 100, 120, 150, 200])
        spot_at_maturity = forward_price + random.choice([-30, -20, -10, 10, 20, 30])
        contracts = random.choice([1, 5, 10, 20])
        answer = (spot_at_maturity - forward_price) * contracts
        question = (
            f"Inwestor zajal dluga pozycje w kontrakcie forward na {contracts} szt. instrumentu bazowego. "
            f"Cena forward wynosi {forward_price:.2f} PLN, a cena spot w terminie wygasniecia {spot_at_maturity:.2f} PLN. "
            "Oblicz wynik pozycji dlugiej. Podaj wynik w PLN."
        )
        explanation = f"Long forward: wynik = (S_T - F) * liczba = ({spot_at_maturity:.2f} - {forward_price:.2f}) * {contracts} = {answer:.2f} PLN."
        xp_reward = 10
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "derivative_forward_short_payoff":
        forward_price = random.choice([80, 100, 120, 150, 200])
        spot_at_maturity = forward_price + random.choice([-30, -20, -10, 10, 20, 30])
        contracts = random.choice([1, 5, 10, 20])
        answer = (forward_price - spot_at_maturity) * contracts
        question = (
            f"Inwestor zajal krotka pozycje w kontrakcie forward na {contracts} szt. instrumentu bazowego. "
            f"Cena forward wynosi {forward_price:.2f} PLN, a cena spot w terminie wygasniecia {spot_at_maturity:.2f} PLN. "
            "Oblicz wynik pozycji krotkiej. Podaj wynik w PLN."
        )
        explanation = f"Short forward: wynik = (F - S_T) * liczba = ({forward_price:.2f} - {spot_at_maturity:.2f}) * {contracts} = {answer:.2f} PLN."
        xp_reward = 10
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "derivative_call_payoff":
        strike = random.choice([80, 100, 120, 150])
        spot_at_maturity = strike + random.choice([-30, -10, 0, 10, 25, 40])
        premium = random.choice([4, 6, 8, 10, 12])
        answer = max(spot_at_maturity - strike, 0.0) - premium
        question = (
            f"Inwestor kupil opcje call z cena wykonania {strike:.2f} PLN i premia {premium:.2f} PLN. "
            f"W dniu wygasniecia cena instrumentu bazowego wynosi {spot_at_maturity:.2f} PLN. "
            "Oblicz zysk/strate nabywcy opcji z uwzglednieniem premii. Podaj wynik w PLN."
        )
        explanation = f"Long call: max(S_T-K,0)-premia = max({spot_at_maturity:.2f}-{strike:.2f},0)-{premium:.2f} = {answer:.2f} PLN."
        xp_reward = 15
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "derivative_put_payoff":
        strike = random.choice([80, 100, 120, 150])
        spot_at_maturity = strike + random.choice([-40, -25, -10, 0, 10, 30])
        premium = random.choice([4, 6, 8, 10, 12])
        answer = max(strike - spot_at_maturity, 0.0) - premium
        question = (
            f"Inwestor kupil opcje put z cena wykonania {strike:.2f} PLN i premia {premium:.2f} PLN. "
            f"W dniu wygasniecia cena instrumentu bazowego wynosi {spot_at_maturity:.2f} PLN. "
            "Oblicz zysk/strate nabywcy opcji z uwzglednieniem premii. Podaj wynik w PLN."
        )
        explanation = f"Long put: max(K-S_T,0)-premia = max({strike:.2f}-{spot_at_maturity:.2f},0)-{premium:.2f} = {answer:.2f} PLN."
        xp_reward = 15
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "derivative_protective_put_payoff":
        stock_buy = random.choice([70, 80, 100, 120])
        strike = stock_buy
        spot_at_maturity = stock_buy + random.choice([-30, -15, 0, 15, 30])
        put_premium = random.choice([3, 5, 7, 9])
        answer = (spot_at_maturity - stock_buy) + max(strike - spot_at_maturity, 0.0) - put_premium
        question = (
            f"Inwestor kupil akcje po {stock_buy:.2f} PLN i jednoczesnie kupil opcje put z cena wykonania {strike:.2f} PLN "
            f"za premie {put_premium:.2f} PLN. W dniu wygasniecia akcja kosztuje {spot_at_maturity:.2f} PLN. "
            "Oblicz wynik strategii protective put na jednej akcji. Podaj wynik w PLN."
        )
        explanation = (
            f"Wynik akcji = {spot_at_maturity:.2f}-{stock_buy:.2f}; wynik put = max({strike:.2f}-{spot_at_maturity:.2f},0)-{put_premium:.2f}; "
            f"razem = {answer:.2f} PLN."
        )
        xp_reward = 20
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "derivative_put_call_parity_call":
        while True:
            stock_price = random.choice([80, 100, 120, 150])
            strike = random.choice([80, 100, 120, 150])
            rate_pct = random.choice([4, 5, 6, 8])
            years = random.choice([1, 2, 3])
            pv_strike = strike / ((1.0 + rate_pct / 100.0) ** years)
            answer = random.choice([5, 7, 9, 12, 15, 18])
            put_price = answer + pv_strike - stock_price
            if put_price > 1.0:
                break
        question = (
            f"Dla europejskich opcji na akcje bez dywidendy: S={stock_price:.2f}, K={strike:.2f}, "
            f"cena put={put_price:.2f}, stopa wolna od ryzyka {rate_pct:.2f}%, T={years} lat. "
            "Korzystajac z parytetu put-call oblicz cene call. Podaj wynik w PLN."
        )
        explanation = f"c + PV(K) = p + S, wiec c = {put_price:.2f}+{stock_price:.2f}-{pv_strike:.2f} = {answer:.2f} PLN."
        xp_reward = 25
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "derivative_put_call_parity_put":
        while True:
            stock_price = random.choice([80, 100, 120, 150])
            strike = random.choice([80, 100, 120, 150])
            rate_pct = random.choice([4, 5, 6, 8])
            years = random.choice([1, 2, 3])
            pv_strike = strike / ((1.0 + rate_pct / 100.0) ** years)
            answer = random.choice([4, 6, 8, 10, 12, 15])
            call_price = answer + stock_price - pv_strike
            if call_price > 1.0:
                break
        question = (
            f"Dla europejskich opcji na akcje bez dywidendy: S={stock_price:.2f}, K={strike:.2f}, "
            f"cena call={call_price:.2f}, stopa wolna od ryzyka {rate_pct:.2f}%, T={years} lat. "
            "Korzystajac z parytetu put-call oblicz cene put. Podaj wynik w PLN."
        )
        explanation = f"p = c + PV(K) - S = {call_price:.2f}+{pv_strike:.2f}-{stock_price:.2f} = {answer:.2f} PLN."
        xp_reward = 25
        answer_unit = "PLN"
        tolerance = 0.01
    elif template == "derivative_delta_new_option_price":
        while True:
            option_price = random.choice([4.20, 5.80, 7.50, 9.60, 12.40])
            delta = random.choice([-0.65, -0.45, -0.25, 0.35, 0.50, 0.70])
            old_spot = random.choice([40, 60, 80, 100])
            spot_change = random.choice([-8, -5, -3, 3, 5, 8])
            new_spot = old_spot + spot_change
            answer = option_price + delta * spot_change
            if answer > 0.25:
                break
        question = (
            f"Cena opcji wynosi {option_price:.2f} PLN, delta = {delta:.2f}. "
            f"Cena instrumentu bazowego zmienia sie z {old_spot:.2f} PLN do {new_spot:.2f} PLN. "
            "Oszacuj nowa cene opcji metoda delty. Podaj wynik w PLN."
        )
        explanation = f"Nowa cena ~= {option_price:.2f} + {delta:.2f} * ({new_spot:.2f}-{old_spot:.2f}) = {answer:.2f} PLN."
        xp_reward = 20
        answer_unit = "PLN"
        tolerance = 0.05
    elif template == "derivative_delta_estimation":
        old_option = random.choice([4.20, 6.40, 8.50, 10.20])
        delta = random.choice([-0.70, -0.45, 0.35, 0.55, 0.75])
        spot_change = random.choice([-10, -7, -5, 5, 7, 10])
        new_option = old_option + delta * spot_change
        old_spot = random.choice([40, 60, 80, 100])
        new_spot = old_spot + spot_change
        answer = (new_option - old_option) / (new_spot - old_spot)
        question = (
            f"Cena instrumentu bazowego zmienila sie z {old_spot:.2f} PLN do {new_spot:.2f} PLN, "
            f"a cena opcji z {old_option:.2f} PLN do {new_option:.2f} PLN. "
            "Oszacuj wspolczynnik delta. Podaj wynik jako liczbe."
        )
        explanation = f"Delta = zmiana ceny opcji / zmiana ceny bazowej = ({new_option:.2f}-{old_option:.2f})/({new_spot:.2f}-{old_spot:.2f}) = {answer:.2f}."
        xp_reward = 20
        answer_unit = ""
        tolerance = 0.01
    elif template == "portfolio_expected_return":
        weight_a_pct = random.choice([25, 30, 40, 50, 60, 70])
        weight_b_pct = 100 - weight_a_pct
        return_a_pct = random.choice([6, 8, 10, 12, 15, 18])
        return_b_pct = random.choice([3, 5, 7, 9, 11, 14])
        answer = (weight_a_pct / 100.0) * return_a_pct + (weight_b_pct / 100.0) * return_b_pct
        question = (
            f"Portfel sklada sie z aktywa A o wadze {weight_a_pct:.0f}% i oczekiwanej stopie zwrotu {return_a_pct:.2f}% "
            f"oraz aktywa B o wadze {weight_b_pct:.0f}% i oczekiwanej stopie zwrotu {return_b_pct:.2f}%. "
            "Oblicz oczekiwana stope zwrotu portfela. Podaj wynik w procentach."
        )
        explanation = (
            f"E(r_p) = {weight_a_pct/100.0:.2f}*{return_a_pct:.2f}% + "
            f"{weight_b_pct/100.0:.2f}*{return_b_pct:.2f}% = {answer:.2f}%."
        )
        xp_reward = 10
        answer_unit = "%"
        tolerance = 0.05
    elif template == "portfolio_two_asset_std":
        weight_a_pct = random.choice([30, 40, 50, 60, 70])
        weight_b_pct = 100 - weight_a_pct
        std_a_pct = random.choice([10, 12, 15, 18, 22])
        std_b_pct = random.choice([8, 11, 14, 17, 20])
        correlation = random.choice([-0.20, 0.00, 0.25, 0.40, 0.60])
        w_a = weight_a_pct / 100.0
        w_b = weight_b_pct / 100.0
        std_a = std_a_pct / 100.0
        std_b = std_b_pct / 100.0
        variance = (w_a ** 2) * (std_a ** 2) + (w_b ** 2) * (std_b ** 2) + 2 * w_a * w_b * correlation * std_a * std_b
        answer = (variance ** 0.5) * 100.0
        question = (
            f"Portfel sklada sie z aktywa A o wadze {weight_a_pct:.0f}% i odchyleniu standardowym {std_a_pct:.2f}% "
            f"oraz aktywa B o wadze {weight_b_pct:.0f}% i odchyleniu standardowym {std_b_pct:.2f}%. "
            f"Korelacja stop zwrotu wynosi {correlation:.2f}. Oblicz odchylenie standardowe portfela. "
            "Podaj wynik w procentach."
        )
        explanation = (
            "sigma_p = sqrt(wA^2*sA^2 + wB^2*sB^2 + 2*wA*wB*rho*sA*sB) "
            f"= {answer:.2f}%."
        )
        xp_reward = 25
        answer_unit = "%"
        tolerance = 0.05
    elif template == "portfolio_beta_covariance":
        market_variance = random.choice([0.0225, 0.0324, 0.0400, 0.0625])
        beta = random.choice([0.70, 0.85, 1.10, 1.25, 1.40])
        covariance = beta * market_variance
        answer = beta
        question = (
            f"Kowariancja stopy zwrotu aktywa z rynkiem wynosi {covariance:.4f}, "
            f"a wariancja stopy zwrotu rynku wynosi {market_variance:.4f}. "
            "Oblicz bete aktywa beta = cov(i,m) / var(m)."
        )
        explanation = f"beta = {covariance:.4f} / {market_variance:.4f} = {answer:.2f}."
        xp_reward = 15
        answer_unit = ""
        tolerance = 0.01
    elif template == "portfolio_beta_correlation":
        correlation = random.choice([0.35, 0.45, 0.60, 0.75, 0.90])
        std_asset_pct = random.choice([12, 15, 18, 24, 30])
        std_market_pct = random.choice([10, 12, 15, 18])
        answer = correlation * (std_asset_pct / std_market_pct)
        question = (
            f"Korelacja stopy zwrotu aktywa z rynkiem wynosi {correlation:.2f}. "
            f"Odchylenie standardowe aktywa to {std_asset_pct:.2f}%, a rynku {std_market_pct:.2f}%. "
            "Oblicz bete aktywa beta = rho(i,m) * sigma_i / sigma_m."
        )
        explanation = f"beta = {correlation:.2f} * {std_asset_pct:.2f}% / {std_market_pct:.2f}% = {answer:.2f}."
        xp_reward = 15
        answer_unit = ""
        tolerance = 0.01
    elif template == "portfolio_capm_expected_return":
        risk_free_pct = random.choice([3, 4, 5, 6])
        market_return_pct = random.choice([9, 10, 11, 12, 14])
        beta = random.choice([0.75, 0.90, 1.10, 1.25, 1.50])
        answer = risk_free_pct + beta * (market_return_pct - risk_free_pct)
        question = (
            f"Stopa wolna od ryzyka wynosi {risk_free_pct:.2f}%, oczekiwana stopa zwrotu rynku {market_return_pct:.2f}%, "
            f"a beta aktywa {beta:.2f}. Oblicz oczekiwana stope zwrotu wedlug CAPM. Podaj wynik w procentach."
        )
        explanation = f"E(r_i) = {risk_free_pct:.2f}% + {beta:.2f}*({market_return_pct:.2f}%-{risk_free_pct:.2f}%) = {answer:.2f}%."
        xp_reward = 20
        answer_unit = "%"
        tolerance = 0.05
    elif template == "portfolio_wacc":
        debt_to_equity = random.choice([0.25, 0.40, 0.60, 0.80, 1.00])
        cost_equity_pct = random.choice([10, 12, 14, 16, 18])
        cost_debt_pct = random.choice([5, 6, 7, 8, 9])
        tax_pct = random.choice([9, 12, 19])
        weight_equity = 1.0 / (1.0 + debt_to_equity)
        weight_debt = debt_to_equity / (1.0 + debt_to_equity)
        answer = weight_equity * cost_equity_pct + weight_debt * cost_debt_pct * (1.0 - tax_pct / 100.0)
        question = (
            f"Spolka ma relacje D/E = {debt_to_equity:.2f}, koszt kapitalu wlasnego {cost_equity_pct:.2f}%, "
            f"koszt dlugu {cost_debt_pct:.2f}% oraz stope podatku {tax_pct:.2f}%. "
            "Oblicz WACC. Podaj wynik w procentach."
        )
        explanation = (
            f"wE = {weight_equity:.2f}, wD = {weight_debt:.2f}. "
            f"WACC = wE*ke + wD*kd*(1-T) = {answer:.2f}%."
        )
        xp_reward = 25
        answer_unit = "%"
        tolerance = 0.05
    elif template == "portfolio_sharpe_ratio":
        portfolio_return_pct = random.choice([8, 10, 12, 14, 16])
        risk_free_pct = random.choice([3, 4, 5])
        std_portfolio_pct = random.choice([8, 10, 12, 15, 18])
        answer = (portfolio_return_pct - risk_free_pct) / std_portfolio_pct
        question = (
            f"Portfel osiagnal stope zwrotu {portfolio_return_pct:.2f}%, stopa wolna od ryzyka wynosi {risk_free_pct:.2f}%, "
            f"a odchylenie standardowe portfela {std_portfolio_pct:.2f}%. Oblicz wskaznik Sharpe'a."
        )
        explanation = f"Sharpe = ({portfolio_return_pct:.2f}% - {risk_free_pct:.2f}%) / {std_portfolio_pct:.2f}% = {answer:.2f}."
        xp_reward = 15
        answer_unit = ""
        tolerance = 0.01
    elif template == "portfolio_treynor_ratio":
        portfolio_return_pct = random.choice([8, 10, 12, 14, 16])
        risk_free_pct = random.choice([3, 4, 5])
        beta = random.choice([0.70, 0.90, 1.10, 1.30, 1.50])
        answer = (portfolio_return_pct - risk_free_pct) / beta
        question = (
            f"Portfel osiagnal stope zwrotu {portfolio_return_pct:.2f}%, stopa wolna od ryzyka wynosi {risk_free_pct:.2f}%, "
            f"a beta portfela {beta:.2f}. Oblicz wskaznik Treynora. Podaj wynik w procentach."
        )
        explanation = f"Treynor = ({portfolio_return_pct:.2f}% - {risk_free_pct:.2f}%) / {beta:.2f} = {answer:.2f}%."
        xp_reward = 15
        answer_unit = "%"
        tolerance = 0.05
    elif template == "portfolio_jensen_alpha":
        portfolio_return_pct = random.choice([8, 10, 12, 14, 16])
        risk_free_pct = random.choice([3, 4, 5])
        market_return_pct = random.choice([9, 10, 11, 12, 14])
        beta = random.choice([0.70, 0.90, 1.10, 1.30])
        expected_capm = risk_free_pct + beta * (market_return_pct - risk_free_pct)
        answer = portfolio_return_pct - expected_capm
        question = (
            f"Portfel osiagnal stope zwrotu {portfolio_return_pct:.2f}%. Stopa wolna od ryzyka wynosi {risk_free_pct:.2f}%, "
            f"stopa zwrotu rynku {market_return_pct:.2f}%, a beta portfela {beta:.2f}. "
            "Oblicz alfe Jensena. Podaj wynik w procentach."
        )
        explanation = (
            f"alpha = r_p - [r_f + beta*(r_m-r_f)] = {portfolio_return_pct:.2f}% - "
            f"[{risk_free_pct:.2f}% + {beta:.2f}*({market_return_pct:.2f}%-{risk_free_pct:.2f}%)] = {answer:.2f}%."
        )
        xp_reward = 20
        answer_unit = "%"
        tolerance = 0.05
    elif template == "ratio_average_collection_period":
        receivables = random.choice([1140, 12500, 18500, 20000, 40000, 50400])
        turnover = random.choice([5.75, 6.45, 7.50, 11.84, 12.97, 18.72])
        sales = round(receivables * turnover, 2)
        answer = 365.0 / turnover
        question = (
            f"Roczna sprzedaz netto wyniosla {sales:.2f} PLN, a sredni stan naleznosci {receivables:.2f} PLN. "
            "Oblicz sredni okres inkasa przy zalozeniu 365 dni w roku. Podaj wynik w dniach."
        )
        explanation = (
            f"Obrot naleznosciami = {sales:.2f}/{receivables:.2f} = {turnover:.2f}. "
            f"Sredni okres inkasa = 365/{turnover:.2f} = {answer:.2f} dni."
        )
        xp_reward = 15
        answer_unit = "dni"
        tolerance = 0.05
    elif template == "ratio_inventory_processing_period":
        inventory_turnover = random.choice([4.50, 5.20, 6.63, 7.30, 8.40, 9.10])
        answer = 365.0 / inventory_turnover
        question = (
            f"Wskaznik obrotu zapasami wynosi {inventory_turnover:.2f}. "
            "Oblicz sredni okres realizacji obrotu zapasami przy zalozeniu 365 dni w roku. Podaj wynik w dniach."
        )
        explanation = f"Okres obrotu zapasami = 365/{inventory_turnover:.2f} = {answer:.2f} dni."
        xp_reward = 10
        answer_unit = "dni"
        tolerance = 0.05
    elif template == "ratio_cash_conversion_cycle":
        collection_days = random.choice([18, 25, 32, 43, 48])
        inventory_days = random.choice([21, 28, 39, 45, 55])
        payable_days = random.choice([14, 18, 24, 39, 43])
        answer = collection_days + inventory_days - payable_days
        question = (
            f"Sredni okres inkasa wynosi {collection_days} dni, okres obrotu zapasami {inventory_days} dni, "
            f"a okres platnosci zobowiazan {payable_days} dni. Oblicz cykl konwersji gotowki. Podaj wynik w dniach."
        )
        explanation = f"CCC = {collection_days} + {inventory_days} - {payable_days} = {answer:.2f} dni."
        xp_reward = 10
        answer_unit = "dni"
        tolerance = 0.01
    elif template == "ratio_dupont_asset_turnover":
        roe_pct = random.choice([6.43, 9.00, 11.00, 13.50, 15.00])
        net_margin_pct = random.choice([4.00, 7.50, 9.00, 12.00])
        leverage = random.choice([2.10, 2.50, 2.86, 3.10])
        answer = (roe_pct / 100.0) / ((net_margin_pct / 100.0) * leverage)
        question = (
            f"ROE spolki wynosi {roe_pct:.2f}%, marza zysku netto {net_margin_pct:.2f}%, "
            f"a wskaznik dzwigni finansowej {leverage:.2f}. Oblicz wskaznik obrotu aktywami calkowitymi z modelu DuPonta."
        )
        explanation = f"ROE = marza netto * obrot aktywami * dzwignia, wiec obrot aktywami = {answer:.2f}."
        xp_reward = 15
        answer_unit = ""
        tolerance = 0.01
    elif template == "ratio_dupont_net_margin":
        roe_pct = random.choice([9.00, 11.00, 14.00, 18.00])
        asset_turnover = random.choice([0.80, 1.10, 1.50, 2.00])
        leverage = random.choice([1.80, 2.20, 2.50, 3.00])
        answer = (roe_pct / 100.0) / (asset_turnover * leverage) * 100.0
        question = (
            f"ROE spolki wynosi {roe_pct:.2f}%, obrot aktywami calkowitymi {asset_turnover:.2f}, "
            f"a dzwignia finansowa {leverage:.2f}. Oblicz marze zysku netto z modelu DuPonta. Podaj wynik w procentach."
        )
        explanation = f"Marza netto = ROE/(obrot aktywami*dzwignia) = {answer:.2f}%."
        xp_reward = 15
        answer_unit = "%"
        tolerance = 0.05
    elif template == "ratio_gross_profit_margin":
        sales = random.choice([83000, 100000, 125000, 198000, 357000])
        cogs = random.choice([34000, 40000, 45000, 84000, 265000])
        if cogs >= sales:
            cogs = round(sales * random.choice([0.35, 0.45, 0.60]), 2)
        answer = (sales - cogs) / sales * 100.0
        question = (
            f"Sprzedaz netto wyniosla {sales:.2f} PLN, a koszt sprzedanych dobr {cogs:.2f} PLN. "
            "Oblicz marze zysku brutto. Podaj wynik w procentach."
        )
        explanation = f"Marza brutto = ({sales:.2f}-{cogs:.2f})/{sales:.2f} = {answer:.2f}%."
        xp_reward = 10
        answer_unit = "%"
        tolerance = 0.05
    elif template == "ratio_gordon_trailing_pe":
        retention_pct = random.choice([34, 45, 55, 65, 72])
        growth_pct = random.choice([2.0, 3.0, 4.0, 5.0, 6.0])
        required_return_pct = growth_pct + random.choice([4.0, 5.0, 6.0, 7.0])
        payout = 1.0 - retention_pct / 100.0
        answer = payout * (1.0 + growth_pct / 100.0) / ((required_return_pct - growth_pct) / 100.0)
        question = (
            f"Stopa zyskow zatrzymanych wynosi {retention_pct:.2f}%, oczekiwana stopa wzrostu dywidendy {growth_pct:.2f}%, "
            f"a wymagana stopa zwrotu {required_return_pct:.2f}%. Oblicz wsteczny wskaznik P/E w modelu Gordona."
        )
        explanation = f"P/E = payout*(1+g)/(k-g) = {payout:.2f}*(1+{growth_pct/100.0:.4f})/({(required_return_pct-growth_pct)/100.0:.4f}) = {answer:.2f}."
        xp_reward = 25
        answer_unit = ""
        tolerance = 0.01
    elif template == "ratio_gordon_required_return":
        retention_pct = random.choice([30, 40, 55, 65, 75])
        growth_pct = random.choice([3.0, 4.0, 5.0, 6.0])
        pe = random.choice([5.4, 6.5, 7.2, 8.4, 10.2])
        payout = 1.0 - retention_pct / 100.0
        answer = (payout * (1.0 + growth_pct / 100.0) / pe + growth_pct / 100.0) * 100.0
        question = (
            f"Stopa zyskow zatrzymanych wynosi {retention_pct:.2f}%, wsteczny P/E = {pe:.2f}, "
            f"a oczekiwana stopa wzrostu dywidendy {growth_pct:.2f}%. Oblicz wymagana stope zwrotu. Podaj wynik w procentach."
        )
        explanation = f"k = payout*(1+g)/P/E + g = {answer:.2f}%."
        xp_reward = 25
        answer_unit = "%"
        tolerance = 0.05
    elif template == "ratio_gordon_retention_rate":
        retention_pct = random.choice([18, 34, 45, 63, 72])
        growth_pct = random.choice([2.0, 3.0, 5.0, 6.0])
        required_return_pct = growth_pct + random.choice([4.0, 5.0, 6.0, 7.0])
        payout = 1.0 - retention_pct / 100.0
        pe = payout * (1.0 + growth_pct / 100.0) / ((required_return_pct - growth_pct) / 100.0)
        answer = retention_pct
        question = (
            f"Wsteczny P/E spolki wynosi {pe:.2f}, oczekiwana stopa wzrostu dywidendy {growth_pct:.2f}%, "
            f"a wymagana stopa zwrotu {required_return_pct:.2f}%. Oblicz stope zyskow zatrzymanych. Podaj wynik w procentach."
        )
        explanation = f"Retention = 1 - P/E*(k-g)/(1+g) = {answer:.2f}%."
        xp_reward = 25
        answer_unit = "%"
        tolerance = 0.05
    elif template == "ratio_dol":
        quantity = random.choice([7500, 8000, 20000, 30000, 60000])
        price = random.choice([10, 20, 30, 50])
        variable_cost = random.choice([4, 8, 12, 20])
        if variable_cost >= price:
            variable_cost = price * 0.5
        contribution = quantity * (price - variable_cost)
        fixed_costs = contribution * random.choice([0.10, 0.20, 0.35, 0.45])
        answer = contribution / (contribution - fixed_costs)
        question = (
            f"Spolka sprzedaje {quantity} szt. produktu po {price:.2f} PLN, koszt zmienny jednostkowy wynosi {variable_cost:.2f} PLN, "
            f"a koszty stale {fixed_costs:.2f} PLN. Oblicz stopien dzwigni operacyjnej DOL."
        )
        explanation = f"DOL = marza pokrycia / EBIT = {contribution:.2f}/({contribution:.2f}-{fixed_costs:.2f}) = {answer:.2f}."
        xp_reward = 20
        answer_unit = ""
        tolerance = 0.01
    elif template == "ratio_fixed_costs_from_dol":
        quantity = random.choice([8000, 20000, 30000, 60000])
        price = random.choice([4, 10, 20, 30])
        variable_cost = random.choice([2, 4, 5, 12, 20])
        if variable_cost >= price:
            variable_cost = price * 0.5
        dol = random.choice([1.10, 1.14, 1.25, 1.50, 2.00])
        contribution = quantity * (price - variable_cost)
        answer = contribution * (dol - 1.0) / dol
        question = (
            f"Spolka sprzedaje {quantity} szt. produktu po {price:.2f} PLN, koszt zmienny jednostkowy wynosi {variable_cost:.2f} PLN, "
            f"a DOL = {dol:.2f}. Oblicz calkowite koszty stale. Podaj wynik w PLN."
        )
        explanation = f"DOL = C/(C-F), wiec F = C*(DOL-1)/DOL = {answer:.2f} PLN."
        xp_reward = 25
        answer_unit = "PLN"
        tolerance = 1.0
    elif template == "ratio_dtl":
        dol = random.choice([1.20, 1.50, 2.00, 2.50, 3.00])
        dfl = random.choice([1.10, 1.25, 1.50, 2.00])
        answer = dol * dfl
        question = (
            f"Stopien dzwigni operacyjnej DOL wynosi {dol:.2f}, a stopien dzwigni finansowej DFL {dfl:.2f}. "
            "Oblicz stopien dzwigni calkowitej DTL."
        )
        explanation = f"DTL = DOL * DFL = {dol:.2f} * {dfl:.2f} = {answer:.2f}."
        xp_reward = 10
        answer_unit = ""
        tolerance = 0.01
    elif template == "ratio_cost_of_debt_from_roc":
        debt = random.choice([12000, 25000, 43000, 50000, 63000])
        equity = random.choice([21000, 25000, 40000, 65000, 105000])
        roe_pct = random.choice([10, 12, 15, 17, 20])
        tax_pct = random.choice([5, 8, 10, 14, 19])
        cost_debt_pct = random.choice([3, 5, 7, 9, 11])
        roc_pct = ((roe_pct / 100.0) * equity + (cost_debt_pct / 100.0) * debt * (1.0 - tax_pct / 100.0)) / (debt + equity) * 100.0
        answer = cost_debt_pct
        question = (
            f"ROE wynosi {roe_pct:.2f}%, ROC {roc_pct:.2f}%, D = {debt:.2f} PLN, E = {equity:.2f} PLN, "
            f"a stopa podatku {tax_pct:.2f}%. Oblicz koszt kapitalu obcego. Podaj wynik w procentach."
        )
        explanation = (
            "ROC = (ROE*E + i*D*(1-T))/(D+E), wiec "
            f"i = [ROC*(D+E)-ROE*E]/[D*(1-T)] = {answer:.2f}%."
        )
        xp_reward = 35
        answer_unit = "%"
        tolerance = 0.05
    elif template == "discounted_payback":
        outlay = random.choice([10000, 15000, 20000])
        cashflow = random.choice([3500, 4500, 6000, 7500])
        rate_pct = random.choice([8, 10, 12])
        rate = rate_pct / 100.0
        cumulative = 0.0
        full_years = 0
        discounted_last = 0.0
        for year in range(1, 8):
            discounted_last = cashflow / ((1.0 + rate) ** year)
            if cumulative + discounted_last >= outlay:
                break
            cumulative += discounted_last
            full_years = year
        answer = full_years + ((outlay - cumulative) / discounted_last)
        question = (
            f"Projekt wymaga nakladu {outlay:.2f} PLN i generuje {cashflow:.2f} PLN "
            f"na koniec kazdego roku. Stopa dyskontowa wynosi {rate_pct:.2f}%. "
            "Oblicz zdyskontowany okres zwrotu. Podaj wynik w latach."
        )
        explanation = (
            f"Po {full_years} pelnych latach odzyskano zdyskontowane {cumulative:.2f} PLN. "
            f"Brakujaca kwota / zdyskontowany przeplyw kolejnego roku daje {answer:.2f} lat."
        )
        xp_reward = 30
        answer_unit = "lat"
        tolerance = 0.05
    else:
        raise ValueError(f"Nieznany szablon zadania: {template}")

    choices, correct_choice = build_numeric_choices(answer, answer_unit)
    answer_mode = random.choice(["choice", "numeric"])

    return {
        "mode": "exercise",
        "department": department,
        "topic": topic,
        "question": question,
        "answer_value": round(answer, 2),
        "answer_pct": round(answer, 2),
        "answer_unit": answer_unit,
        "tolerance": tolerance,
        "tolerance_pct": tolerance,
        "xp_reward": xp_reward,
        "explanation": explanation,
        "choices": choices,
        "correct_choice": correct_choice,
        "answer_mode": answer_mode,
        "exercise_id": f"{department}:{template}:{int(time.time() * 1000)}",
    }


def main(argv: list[str]) -> int:
    try:
        if len(argv) < 2:
            raise ValueError("Uzycie: financial_calculator.py investment|stock <argumenty>")

        mode = argv[1]
        if mode == "investment":
            emit(calculate_investment(argv[2:]))
        elif mode == "stock":
            emit(calculate_stock(argv[2:]))
        elif mode == "exercise":
            department = argv[2] if len(argv) > 2 else ""
            emit(generate_financial_math_exercise(department))
        else:
            raise ValueError(f"Nieznany tryb kalkulatora: {mode}")
        return 0
    except Exception as exc:
        emit({"error": str(exc)})
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
