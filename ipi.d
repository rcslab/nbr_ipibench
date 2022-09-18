#!/usr/sbin/dtrace -s
#pragma D option quiet

syscall::kill:entry
{
    self->ksys = timestamp;
}

syscall::kill:return
{
    self->ksys_done = timestamp;
    @ksys = lquantize(self->ksys_done - self->ksys, 0, 20000, 100);
}

syscall::thr_kill:entry
{
    self->ksys = timestamp;
}

syscall::thr_kill:return
{
    self->ksys_done = timestamp;
    @ksys = lquantize(self->ksys_done - self->ksys, 0, 20000, 100);
}


fbt:kernel:tdsendsignal:entry
{
    self->tdss = timestamp;
}

fbt:kernel:tdsendsignal:return
{
    self->tdss_done = timestamp;
    @tdss = lquantize(self->tdss_done - self->tdss, 0, 10000, 100);
}

fbt:kernel:ipi_cpu:entry
{
    self->ipi = timestamp;
}

fbt:kernel:ipi_cpu:return
{
    self->ipi_done = timestamp;
    @ipicpu = lquantize(self->ipi_done - self->ipi, 0, 10000, 100);
}

END
{
    printf("kill\n");
    printa(@ksys);
    printf("tdsendsignal\n");
    printa(@tdss);
    printf("ipi_cpu\n");
    printa(@ipicpu);
}

