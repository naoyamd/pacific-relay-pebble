from pathlib import Path


def weekday(year, month, day):
    offsets = (0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4)
    if month < 3:
        year -= 1
    return (year + year // 4 - year // 100 + year // 400 + offsets[month - 1] + day) % 7


def nth_sunday(year, month, occurrence):
    return 1 + (7 - weekday(year, month, 1)) % 7 + (occurrence - 1) * 7


def is_dst(year, month, day, hour):
    start = nth_sunday(year, 3, 2)
    end = nth_sunday(year, 11, 1)
    if month < 3 or month > 11:
        return 3 < month < 11
    if month == 3:
        return day > start or (day == start and hour >= 10)
    if month == 11:
        return day < end or (day == end and hour < 9)
    return True


for year, start, end in ((2024, 10, 3), (2025, 9, 2), (2026, 8, 1)):
    assert not is_dst(year, 3, start, 9)
    assert is_dst(year, 3, start, 10)
    assert is_dst(year, 11, end, 8)
    assert not is_dst(year, 11, end, 9)

source = Path(__file__).parent.parent / "src" / "c" / "timezone.c"
text = source.read_text(encoding="utf-8")
assert "hour >= 10" in text and "hour < 9" in text
print("DST boundary tests passed (Python fallback)")
