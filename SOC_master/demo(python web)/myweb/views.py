from rest_framework.viewsets import ModelViewSet
from .models import BatteryOutputList
from .models import BatteryInfo
from .serializers import BOListModelSerializer
from .serializers import BInfoModelSerializer


# Create your views here.

class BOListModelViewSet(ModelViewSet):  # BatteryOutputList
    queryset = BatteryOutputList.objects.all()
    serializer_class = BOListModelSerializer


class BInfoModelViewSet(ModelViewSet):
    queryset = BatteryInfo.objects.all()
    serializer_class = BInfoModelSerializer
