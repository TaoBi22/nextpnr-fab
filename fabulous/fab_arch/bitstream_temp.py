from write_fasm import *
#from simple_config import K

# Need to tell FASM generator how to write parameters
# (celltype, parameter) -> ParameterConfig
param_map = {
	("FABULOUS_LC", "K"): ParameterConfig(write=False),
	("FABULOUS_LC", "INIT"): ParameterConfig(write=True, numeric=True, width=16),
	("FABULOUS_LC", "DFF_ENABLE"): ParameterConfig(write=True, numeric=True, width=1),

	("IO_1_bidirectional_frame_config_pass", "INPUT_USED"): ParameterConfig(write=True, numeric=True, width=1),
	("IO_1_bidirectional_frame_config_pass", "OUTPUT_USED"): ParameterConfig(write=True, numeric=True, width=1),
	("IO_1_bidirectional_frame_config_pass", "ENABLE_USED"): ParameterConfig(write=True, numeric=True, width=1),
}

with open("sequential_16bit.fasm", "w") as f:
	write_fasm(ctx, param_map, f)
