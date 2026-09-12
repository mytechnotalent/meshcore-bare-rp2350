"""Unit test adapter for VS Code Test Explorer."""
import subprocess
import sys
import unittest

_CACHED_OUTPUT = ""


def _get_unity_output() -> str:
    """
    Execute native tests and return stdout.

    Parameters
    ----------
    None

    Returns
    -------
    str
        Standard output from native test suite.
    """
    global _CACHED_OUTPUT
    if not _CACHED_OUTPUT:
        cmd = [sys.executable, "scripts/run_tests.py"]
        res = subprocess.run(cmd, capture_output=True, text=True)
        _CACHED_OUTPUT = res.stdout
    return _CACHED_OUTPUT


def _assert_unity_pass(test_name: str) -> None:
    """
    Assert that a named Unity test passed.

    Parameters
    ----------
    test_name : str
        Name of Unity test function.

    Returns
    -------
    None
    """
    output = _get_unity_output()
    expected = f":{test_name}:PASS"
    assert expected in output, f"{test_name} did not pass in Unity output"


class TestMeshCoreFirmware(unittest.TestCase):
    """Test cases for RP2350 bare MeshCore companion firmware."""

    def test_01_config_constants(self) -> None:
        """
        Verify configuration constants and sizes.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_config_constants")

    def test_02_channel_init(self) -> None:
        """
        Verify default channel initialization.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_channel_init")

    def test_03_channel_capacity_and_boundary(self) -> None:
        """
        Verify channel table capacity and boundary conditions.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_channel_capacity_and_boundary")

    def test_04_channel_deletion(self) -> None:
        """
        Verify channel deletion and slot compaction.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_channel_deletion")

    def test_05_v3_compact_export_import(self) -> None:
        """
        Verify V3 compact serialization export and import round-trip.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_v3_compact_export_import")

    def test_06_contact_capacity_and_lru_eviction(self) -> None:
        """
        Verify contact table capacity and LRU eviction policy.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_contact_capacity_and_lru_eviction")

    def test_07_channel_sanitization_and_validation(self) -> None:
        """
        Verify channel name sanitization and key validation.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_channel_sanitization_and_validation")

    def test_08_trace_packet_encode_decode(self) -> None:
        """
        Verify trace packet encoding and decoding round-trip.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_trace_packet_encode_decode")

    def test_09_trace_path_hop_validation(self) -> None:
        """
        Verify trace path hop counting and boundary validation.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_trace_path_hop_validation")

    def test_10_companion_text_type_range(self) -> None:
        """
        Verify companion text message type enumeration ranges.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_companion_text_type_range")

    def test_11_trace_push_frame_structure(self) -> None:
        """
        Verify companion trace push frame structure formatting.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_trace_push_frame_structure")

    def test_12_mesh_packet_transport_route_encode_decode(self) -> None:
        """
        Verify transport routing envelope encoding and decoding.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_mesh_packet_transport_route_encode_decode")

    def test_13_mesh_packet_validation_errors(self) -> None:
        """
        Verify mesh packet validation and corrupt payload rejections.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_mesh_packet_validation_errors")

    def test_14_datagram_request_direct_and_flood(self) -> None:
        """
        Verify datagram request framing for direct and flood modes.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_datagram_request_direct_and_flood")

    def test_15_datagram_request_validation(self) -> None:
        """
        Verify datagram request parameter validation and boundaries.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_datagram_request_validation")

    def test_16_datagram_text_and_group_text(self) -> None:
        """
        Verify datagram payload formatting for direct and group text.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_datagram_text_and_group_text")

    def test_17_companion_status_push_frame(self) -> None:
        """
        Verify companion node status push frame formatting.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_companion_status_push_frame")

    def test_18_companion_opcode_constants(self) -> None:
        """
        Verify companion protocol opcode and response constants.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        _assert_unity_pass("test_companion_opcode_constants")

    def test_19_audit_blank_lines(self) -> None:
        """
        Verify zero blank lines in C function bodies.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        cmd = [sys.executable, "scripts/audit_blank_lines.py"]
        res = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(res.returncode, 0, res.stdout + res.stderr)

    def test_20_audit_python_standard(self) -> None:
        """
        Verify Python tooling standard compliance.

        Parameters
        ----------
        None

        Returns
        -------
        None
        """
        cmd = [sys.executable, "scripts/audit_python_standard.py"]
        res = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(res.returncode, 0, res.stdout + res.stderr)


if __name__ == "__main__":
    unittest.main()
